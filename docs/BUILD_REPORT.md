# Informe de construcción / prueba — 2026-10-31

## Archivos realmente compilados

- Biblioteca de pruebas Linux x86-64 con `BINK_WITH_FFMPEG=OFF`: `libbink_open_test.so`.
- DLL Win32 x86 con `BINK_WITH_FFMPEG=OFF`: `build-win32/binkw32.dll`, copiada a `bin/binkw32.dll`.
- Exportaciones PE32: **66/66 símbolos, ordinales y decoraciones coincidentes** mediante `tools/check_def.py`.
- CTest Linux: **6 de 6 pasan** (también bajo UBSan) (`core`, `export_manifest`, `pixels`, `audio_mix`, `surface_mapping`, `sound_driver_api`).

- El programa de integración `bink_probe` compila, pero no se ejecutó con Bink real.

## Lo que no se ejecutó

- No se pudo compilar el backend FFmpeg de vídeo/audio por no disponer de los **headers** y libs de desarrollo para compilarlo en este entorno.
- No se ha reproducido audio WaveOut (necesita ejecución Windows real).
- No se ha decodificado ningún archivo Bink real. Solo contenedores Bink 1 sintéticos no decodificables.
- No hay pruebas diferenciales con la DLL original ni validación de `struct` ABI en tiempo de ejecución.

## Comportamientos implementados pero sin prueba de dispositivo

- WinMM `waveOutOpen`, envío de PCM16 en cola de 16 cabeceras, pausa/reanudación, cierre.
- Blit GDI de rectángulos con escalado; consulta de posición/área cliente de una ventana Win32.
- Superficies DirectDraw/D3D8/D3D9 mediante vtable COM y descriptores públicos (mapeos experimentales).

## Advertencia de distribución

`bin/binkw32.dll` **NO tiene soporte de decodificación Bink** por estar compilada sin FFmpeg; no se debe etiquetar como un reemplazo funcional completo.

## Actualización: workflow FFmpeg estático Win32 x86 (2026-10-31)

- Nuevo `.github/workflows/build.yml`: Ubuntu 24.04, compilación cruzada MinGW-w64 i686, FFmpeg 7.1.5 estático, CMake con `BINK_WITH_FFMPEG=ON` y `BINK_FFMPEG_STATIC=ON`.
- Archivo final único: `binkw32.dll`, en GitHub Actions mediante `upload-artifact@v7` + `archive: false`, y como recurso adjunto sin ZIP en GitHub Releases para etiquetas `v*`.
- Se añadió `tools/check_win32_dll.py`, que comprueba PE32/x86, 66 exportaciones, los parámetros CMake FFmpeg y ausencia de dependencias de DLL multimedia externas.
- Validación local: el verificador de PE funciona sobre la DLL Win32 **anterior, sin FFmpeg**, y las seis pruebas nativas sin FFmpeg siguen pasando. Comprobación real de la compilación cruzada estática y de decodificación Bink: **pendiente de ejecutar en GitHub Actions**.
