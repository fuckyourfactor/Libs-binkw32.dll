# bink-open — Reimplementación independiente experimental de `binkw32.dll`

**No es todavía un reemplazo 100% compatible ni una DLL con compatibilidad de reproducción verificada en rFactor.** Este proyecto busca interoperabilidad con el binario Win32 x86 proporcionado (Bink 1, año 2005). El código original de este repositorio es MIT y **no carga ni redirige a la DLL propietaria**. La ruta multimedia utiliza el proyecto abierto FFmpeg, con sus propias dependencias y licencias LGPL/GPL según la configuración.

## GitHub Actions: generar un único `binkw32.dll` con FFmpeg integrado

El workflow [`.github/workflows/build.yml`](.github/workflows/build.yml) compila **Windows PE32 x86** desde Ubuntu con MinGW-w64. Descarga **FFmpeg 7.1.5**, lo compila como cinco bibliotecas **estáticas** con solamente el demuxer Bink1 y los decodificadores de vídeo Bink y audio Bink DCT/RDFT. Después construye `binkw32.dll` enlazando FFmpeg dentro del mismo PE y verifica las **66 exportaciones** (nombres decorados + ordinales), la arquitectura y la ausencia de DLL externas no pertenecientes a Windows.

Para utilizarlo en tu repositorio GitHub:

1. Sube **el código fuente de este ZIP** a la rama `main` o `master`, incluyendo la carpeta oculta `.github/workflows`.
2. En GitHub, abre **Actions → Build binkw32.dll (FFmpeg, Windows x86) → Run workflow**. También se ejecuta al hacer `push`.
3. Cuando el job termine correctamente, descarga el artefacto **`binkw32.dll`**. Se publica sin contenedor ZIP gracias a `actions/upload-artifact@v7` con `archive: false`.
4. Si subes un tag `v*`, el workflow intenta publicar igualmente **`binkw32.dll`** como archivo individual en el GitHub Release correspondiente.

**El único archivo publicado por el workflow es `binkw32.dll`**; no incluye `avcodec-*.dll`, `avformat-*.dll`, `libgcc_s_*.dll` ni otra DLL de FFmpeg. Las DLL estándar de Windows siguen siendo dependencias del sistema. El proyecto fuente, los tests y las licencias no se agregan al artefacto binario.

Para ejecutar la misma construcción en Ubuntu 24.04:

```bash
sudo apt-get update
sudo apt-get install -y gcc-mingw-w64-i686-win32 binutils-mingw-w64-i686 \
  cmake ninja-build make pkgconf curl xz-utils python3
./tools/build_ffmpeg_mingw32.sh
export PKG_CONFIG_LIBDIR="$PWD/.build-ffmpeg/prefix/lib/pkgconfig"
export PKG_CONFIG_PATH=""
cmake -S . -B build-win32-ffmpeg -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw32.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBINK_WITH_FFMPEG=ON -DBINK_FFMPEG_STATIC=ON -DBINK_BUILD_TESTS=OFF
cmake --build build-win32-ffmpeg --target binkw32 -j4
python3 tools/check_win32_dll.py build-win32-ffmpeg/binkw32.dll \
  --require-static-ffmpeg --cmake-cache build-win32-ffmpeg/CMakeCache.txt
```

Resultado: `build-win32-ffmpeg/binkw32.dll`. El workflow copia únicamente ese archivo a `dist/`.

**Nivel de verificación:** la configuración del CI, sus scripts y las pruebas existentes se han revisado, pero no se ha ejecutado todavía un job remoto con FFmpeg en este entorno. El éxito de la compilación con FFmpeg queda por confirmar en GitHub Actions; la decodificación con Bink real y la compatibilidad con rFactor tampoco se han validado. La salida sin FFmpeg de versiones anteriores **no debe confundirse con esta nueva compilación**.

**Licencias:** el código de este proyecto es MIT; la distribución del binario que incorpora FFmpeg enlazado estáticamente debe respetar también las obligaciones **LGPL** de FFmpeg, incluyendo los avisos, disponibilidad del código fuente de FFmpeg y la posibilidad de modificar y volver a enlazar la biblioteca. El workflow no habilita componentes GPL o no libres. Consulta https://ffmpeg.org/legal.html antes de distribuir el binario.

---

## Estado comprobado (31 de octubre de 2026)

| Subsistema | Código incluido | Verificación realmente realizada |
|---|---|---|
| PE32 x86 / exportaciones | `.def` con 66 exportaciones decoradas y ordinales | **66/66** coinciden con la muestra en un PE32 compilado |
| Metadatos Bink 1 | Índice, pistas, keyframes, cabeceras | Pruebas unitarias con contenedores sintéticos |
| Vídeo Bink 1 | FFmpeg (`libavformat` + `libavcodec`) opcional, BGRA (`libswscale`) | **No compilado ni probado** con un archivo `.bik` real |
| Audio Bink DCT / RDFT | FFmpeg (`libavcodec` + `libswresample`) opcional: PCM16 por pista/fotograma | **No compilado ni probado** contra Bink real |
| WaveOut | Cola de 16 buffers WinMM, pausa, control de ganancia y balance, adaptador `BINKSND` | Código Win32 compilado, **sin prueba de reproducción en Windows** |
| Superficies RGB/YUV | RGB24/32, 4444, 5551, 555, 565, 655, 664, YUY2, UYVY | Pruebas de conversión CPU; requiere imagen decodificada |
| DirectDraw / Direct3D8/9 | Consulta COM de formatos de superficie conocidos | Prueba mediante interfaces COM simuladas; **sin pruebas GPU reales** |
| GDI | Blit parcial de rectángulos, escalado, consulta de posición de ventana | Código Win32 compilado, sin prueba en una ventana real |
| Equivalencia funcional Bink | Incompleta | **No hay pruebas diferenciales exitosas** |

Los *tests* del repositorio no usan vídeos Bink válidos. No demuestra todavía decodificación audiovisual real. Ver [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md) para el estado individual de las 66 exportaciones y [`docs/BUILD_REPORT.md`](docs/BUILD_REPORT.md) para resultados.

## Compilación anterior y otras variantes

### 1. DLL Win32 x86 reproducible sin SDK de Windows

El modo histórico se compila con Clang + LLD, sin FFmpeg:

```bash
./tools/build_win32_clang.sh
python3 tools/check_def.py src/binkw32.def docs/exports.json build-win32/binkw32.dll
```

**Importante:** esta variante sí exporta las 66 funciones, incluye conversión CPU, introspección experimental COM y controlador WaveOut, **pero no contiene los decodificadores Bink**. `BinkDoFrame` rechaza un `.bik` en esta versión. No reemplaza funcionalmente la DLL original.

### 2. Compilación con decodificadores Bink de FFmpeg

Para intentar vídeo y audio reales, compilar **Windows x86** con las cabeceras y las bibliotecas de desarrollo de FFmpeg **para Windows x86 de la misma arquitectura** y con `pkg-config`:

```bash
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBINK_WITH_FFMPEG=ON -DBINK_BUILD_TESTS=OFF
cmake --build build-win --config Release
```

Requiere módulos `libavformat`, `libavcodec`, `libavutil`, `libswscale` y `libswresample`. El ejecutable dependiente requiere las **DLL de FFmpeg** correspondientes, no necesita la DLL original de Bink. El empaquetado y los términos legales de las bibliotecas FFmpeg son responsabilidad de la distribución de FFmpeg escogida.

**Esta configuración con FFmpeg aún no se pudo compilar aquí**, ya que este entorno carece de las cabeceras/bibliotecas de desarrollo necesarias; el nuevo workflow las compila en GitHub. La interfaz de FFmpeg y el audio Bink real no están validados. Se recomienda usar una versión de FFmpeg actual (API con `AVChannelLayout` y `swr_alloc_set_opts2`).

### Compilación Linux para pruebas

```bash
cmake -S . -B build -DBINK_BUILD_TESTS=ON -DBINK_WITH_FFMPEG=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Linux produce `libbink_open_test.so`, **no** una DLL Win32 de reemplazo.

## Interfaz básica

```c
/* Requiere haber enlazado la biblioteca con el formato ABI Win32 x86. */
BinkSetSoundSystem(BinkOpenWaveOut, 0); /* audio de salida, solo Windows */
HBINK b = BinkOpen("video.bik", 0);
if (!b) {
    /* Diagnóstico: BinkGetError() */
}
/* Después, para cada fotograma: BinkWait / BinkDoFrame /
   BinkCopyToBuffer / BinkNextFrame. */
```

En la compilación con FFmpeg, `BinkDoFrame` decodifica vídeo y, si se seleccionó WaveOut, puede enviar PCM de la pista seleccionada a WinMM. Se intenta mantener continuidad predictiva al navegar; **la sincronización precisa A/V, el retorno de códigos y los detalles de llamadas de la DLL original siguen sin verificar**.

`BinkOpenTrack` junto con `BinkGetTrackData` decodifica PCM16 por fotograma y comprueba que el tamaño no supere `MaxSize`; sin FFmpeg reporta error. El audio puede seleccionarse por ID con `BinkSetSoundTrack`. Los controles de pan y volumen para WinMM son una aproximación (valor nominal 32768, pan central 32768) y no están validados contra la DLL original.

## Limitaciones técnicas concretas

- No se ha demostrado igualdad de estructuras binarias con el archivo de 2005. Algunas aplicaciones inspeccionan campos internos; esto puede romper interoperabilidad aunque el importador encuentre las 66 funciones.
- No hay reproducción Bink 2, DirectSound nativo, Miles Sound System, custom I/O, IO de fondo, asignación de buffers externos YUV ni APIs completas de paleta/cursor/resolución.
- La salida gráfica Win32 implementada utiliza un DIB de CPU, no una superficie DirectDraw real; no sustituye un dispositivo Direct3D ni modifica la resolución global.
- La ruta FFmpeg no integra sincronización por reloj de hardware ni recuperación completa frente a errores y saltos; reiniciar el demuxer en búsqueda inversa es costoso.
- `BinkDoFrame` sin FFmpeg falla ante un vídeo Bink. Los binarios antiguos disponibles fuera del workflow son de ese tipo; el código del ZIP permite compilar la nueva variante con FFmpeg.
- Las rutas de audio reales no se probaron con archivos `.bik` de procedencia legal; tampoco se ejecutó Wine ni Windows aquí.

## Comprobación con archivos reales

Con la variante `BINK_WITH_FFMPEG=ON` puedes usar el ejecutable `bink_probe`:

```bash
bink_probe video_obtenido_legalmente.bik audio_decodificado.pcm
```

Este programa recorre los fotogramas, calcula un checksum FNV1a del BGRA realmente decodificado e intenta extraer el audio PCM16 de la pista cero. El fichero `.pcm` no contiene cabecera WAV. Un error de decodificación termina con estado distinto de cero. **No hemos ejecutado esta comprobación aquí por falta de un `.bik` válido y por no poder compilar las cabeceras FFmpeg.**

## Pruebas faltantes para poder afirmar compatibilidad

1. Compilar `BINK_WITH_FFMPEG=ON` en Windows x86, corregir cualquier problema de integración y comprobar al menos un `.bik` Bink 1 con audio DCT y otro RDFT.
2. Verificar salida BGRA/YUV por fotograma, reproducción WaveOut y secuencia BinkWait→DoFrame→Copy→Next; probar `BinkGetTrackData` con el tamaño exacto del buffer.
3. Comparar en un proceso separado las API observables con la DLL original de 2005, con una batería diferencial que incluya errores, pausa, búsqueda, repetición y volumen.
4. Validar `sizeof`, `offsetof` y disposición de las estructuras públicas con una referencia segura de la versión concreta.
5. Implementar o documentar como incompatibles los restantes subsistemas de la matriz.

## Licencias y referencias

- Código original de `bink-open`: [MIT](LICENSE). No se incluye la DLL original ni código propietario extraído de ella.
- FFmpeg: https://ffmpeg.org/legal.html (sus licencias se aplican por separado).
- Especificación pública / implementación FFmpeg: https://github.com/FFmpeg/FFmpeg/blob/master/libavformat/bink.c y https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/binkaudio.c
- Las interfaces multimedia Win32 utilizadas están documentadas públicamente por Microsoft.
