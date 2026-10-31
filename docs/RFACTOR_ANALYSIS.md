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
