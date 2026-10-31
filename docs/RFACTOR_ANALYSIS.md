# rFactor.exe — análisis de interoperabilidad con binkw32.dll

## Evidencia estática

Se analizó el ejecutable suministrado por el usuario, **sin ejecutarlo ni modificarlo**.
SHA256: `84cb37a9e8e63abaf057af6e4aa2011125a9e915bfbe1f160afc2da0d19d1466`.
Formato: PE32, Intel x86. Importa las bibliotecas `binkw32.dll`, `mss32.dll`,
`d3d9.dll` y `d3dx9_30.dll`, entre otras.

Las 16 importaciones Bink son:

```
_BinkDoFrame@4
_BinkNextFrame@4
_BinkGetRects@8
_BinkCopyToBufferRect@44
_BinkCopyToBuffer@28
_BinkSetVolume@12
_BinkGoto@12
_BinkClose@4
_BinkOpenMiles@4
_BinkSetSoundSystem@8
_BinkGetError@0
_BinkOpenDirectSound@4
_BinkSetSoundTrack@8
_BinkOpen@8
_BinkGetSummary@8
_BinkWait@4
```

Aparecen cadenas relevantes: `Error: %s while setting Miles sound`,
`Error: %s while setting DirectSound`, `Error opening BINK file %s`,
`Error locating BINK file %s`, `UIData\\Movies\\`, `PreGame.bik`.
Estas cadenas no demuestran qué rama ejecuta el juego en cada sistema.

## Defectos encontrados en el código anterior

- `BinkSetSoundSystem` rechazaba ambos callbacks que el ejecutable importa
  (Miles y DirectSound); `BinkOpenMiles` y `BinkOpenDirectSound` devolvían NULL.
- `BinkSetSoundTrack` comparaba exclusivamente IDs de streams, cuando la API
  admite la selección por índices de pistas. En algunas películas podía quedar
  sin pista seleccionada.
- `BinkDoFrame` devolvía error por ausencia de tarjeta WaveOut, backpressure o
  error del decodificador de audio después de haber decodificado la imagen.
- `BinkOpen` aceptaba la apertura aunque FFmpeg no hubiera inicializado el vídeo,
  sin advertir al caller que el handle no podría decodificar.

## Corrección de interoperabilidad

El código integra un backend PCM alternativo basado en WinMM WaveOut para los
selectores Miles y DirectSound. Esta sustitución es independiente de DLLs
propietarias, pero **no implementa las interfaces nativas** de esos motores.
Se mantiene separada la salida de audio del éxito de decodificación de vídeo.
Se añade trazabilidad opcional mediante `BINK_OPEN_TRACE`.

## Verificado y pendiente

- Comparación de las 16 importaciones con `docs/exports.json`: **16/16**.
- Binario Win32 sin FFmpeg recompilado con Clang/LLD: **66/66 exportaciones**.
- Tests nativos de controlador y del núcleo: ejecutados localmente.
- **No se ejecutó rFactor.exe** ni se dispuso de archivos Bink reales.
- **No se compiló el enlace FFmpeg Win32 de GitHub Actions en este entorno.**
  Su workflow permanece disponible y genera exclusivamente `binkw32.dll`.
- El layout exacto del estado privado de `BINK`, variantes del video y las
  superficies Direct3D aún requieren pruebas con archivos y entorno reales.

Para obtener evidencia de un fallo de reproducción del juego, ejecutar rFactor
con la variable de entorno `BINK_OPEN_TRACE` apuntando a un archivo de texto,
y revisar `BinkOpen`, `BinkDoFrame` y `BinkCopyToBufferRect`.

## Corrección adicional: BIKi, lectura en memoria y FFmpeg (31 oct 2026)

El registro `bink-open.log` evidencia llamadas `BinkOpen` con `BIKi` y
`BINKFROMMEMORY` (`0x04000000`), además de `MOVIEFILES\ISILOGO.BIK`
y `MOVIEFILES\PREGAME.BIK` que sí se pudieron leer. Había dos defectos
independientes y reproducibles:

1. `src/container.c` leía `num_audio_tracks` en **offset 44**, pero para Bink 1
   ese campo está en **offset 40**. La tabla empieza en el byte 44, y solamente
   para revisión `BIKk` comienza en 48. El desplazamiento erróneo provocaba
   `BinkOpen invalid Bink header / frame index` en películas legítimas.
2. Se trataba el primer parámetro de `BinkOpen` como cadena incluso si
   `flags & BINKFROMMEMORY`. En este modo es un puntero a una imagen Bink
   completa. Los bytes `BIKi...` vistos en el log son datos binarios, no
   rutas corruptas. Ahora se copia el contenido a memoria propia. En Windows,
   `VirtualQuery` verifica que las páginas de origen sean legibles antes de
   copiar; el contenido todavía debe ser un archivo Bink válido.
3. `decoder_ffmpeg.c` intentaba `avformat_open_input(path)` incluso para
   imágenes Bink pasadas desde memoria. Ahora utiliza `AVIOContext` personalizado
   respaldado por `h->file` para ambas fuentes (archivo y memoria), con
   lectura, acceso aleatorio, y reinicio de decodificador.

El registro también muestra fallos de acceso a
`MOVIEFILES\SRGRANDPRIX05.BIK`,
`UIDATA\MOVIES\RFM\SRGRANDPRIX05.BIK` y `DEMOBINK.BIK`: son **rutas que
no se pudieron leer**, no pruebas de que su formato sea inválido. Comprueba
que esos recursos existan donde los solicita el juego. La implementación no
sintetiza ni sustituye archivos ausentes.

Pruebas nuevas: `BIKi` con la tabla en su desplazamiento correcto,
`BIKk` con campo extra, índice truncado/interno y apertura
`BINKFROMMEMORY | BINKSNDTRACK` sin invocar rutas del sistema.
El código FFmpeg nuevo debe validarse en GitHub Actions y con películas reales
para comprobar que la reproducción final en rFactor ya funcione.

## Corrección ABI: vídeo invisible (análisis directo de la DLL original)

La actualización anterior de este proyecto era **incorrecta**: usó la
estructura de una variante del SDK Bink en vez de verificar los desplazamientos
en `binkw32.dll` de **2005, Bink 1.8f**. En concreto, añadió
`FrameChangePercent` después de `SndSize` y desplazó cuatro bytes los campos
que rFactor puede leer directamente a través de `HBINK`.

### Evidencia del original, no inferencia de SDK genérica

En el desensamblado de la DLL original proporcionada:

- `BinkGetRects` (RVA `0xD190`) usa `lea edi,[esi+0x34]`, la dirección
  de `FrameRects[0]` relativa al puntero `HBINK`.
- La misma función usa `cmp dword ptr [esi+0xB4],-1`, así como
  `mov eax,[esi+0xB4]` al devolver el recuento: **`NumRects` está en 180**.
- `BinkCopyToBufferRect` (RVA `0xB670`) accede a `[esi+0xB8]` como
  puntero a `FrameBuffers`: **offset 184**.
- `BinkGetRects` devuelve 0 cuando los flags incluyen `0x04000000`
  (`BINKSURFACEDIRECT`); se refleja esta rama comprobada.

El cambio elimina el `FrameChangePercent` que no pertenece a este prefijo de
Bink 1.8f y restaura `FrameRects=52`, `NumRects=180`, `FrameBuffers=184`.
Las nuevas aserciones de compilación se aplican sobre la ABI de 32 bits y
hay pruebas de presentación para los desplazamientos reales. Esta diferencia
**sí es reproducible** con la DLL original, aunque el modo exacto en que
rFactor utiliza esos campos todavía no se observó durante una ejecución
con depurador.

### Límites del análisis del ejecutable

Las 16 importaciones Bink del ejecutable están enumeradas arriba e incluyen
`BinkGetRects`, `BinkCopyToBuffer`, `BinkCopyToBufferRect` y `BinkDoFrame`.
La sección `.text` del `rFactor.exe` aportado presenta una entropía de casi
8 bits/byte, indicativa de empaquetado/ofuscación: el desensamblado directo
de esa sección no proporciona un grafo de llamadas fiable en disco.
No se afirma haber reconstruido qué ruta gráfica sigue rFactor en tiempo
real. La prueba concluyente será registrar las llamadas de presentación en
Windows con los archivos `.BIK` reales.

### Diagnóstico disponible

`BINK_OPEN_TRACE` registra los primeros 12 `BinkGetRects` y
`BinkCopyToBufferRect` exitosos por película, independientemente de cuándo
se produzcan, además de fallos. Esto permite distinguir entre fallo de
conversión de píxeles y ausencia de llamadas del host.

El comprobador opcional `tools/audit_original_2005.py` contrasta los offsets
observados con tu DLL original y la tabla de importaciones con tu rFactor:

```
python3 tools/audit_original_2005.py binkw32-original.dll rFactor.exe
```

**Estado:** verificados ABI y pruebas de copia con datos sintéticos, y compilada
la DLL Win32 sin FFmpeg; pendiente de validar vídeo real en rFactor/Windows.

## Corrección de entrega de píxeles (31 oct 2026)

El registro `LOG.txt` posterior a la corrección de la ABI confirma una causa
independiente del vídeo invisible: `BinkNextFrame` actualiza `FrameNum` antes
de que rFactor copie la imagen, pero `BinkGetRects` y `BinkCopyToBufferRect`
rechazaban la operación si `frame_decoded != FrameNum`. El patrón constante
`current=N decoded=N-1` demuestra que el sistema tenía disponible la imagen
y la descartaba al intentar presentarla. Se corrige conservando el último
fotograma decodificado hasta que lo sustituya otra decodificación, o se
invalide mediante un salto `BinkGoto`. Ver `DISPLAY_HANDOFF_FIX.md`.
