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
