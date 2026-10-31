# MinGW + FFmpeg: corrección del fallo de enlazado

El log aportado por el usuario contiene dos problemas simultáneos:

1. **`cannot export _BinkOpen@8: symbol not defined` (66 veces).**
   La tabla `.def` original conserva los nombres MSVC decorados `_Name@Bytes`.
   El enlazador GNU MinGW aplica un prefijo `_` a su búsqueda de símbolos
   internos de las declaraciones `.def`; un nombre interno que empiece por
   `_` puede buscarse con dos guiones bajos. El nuevo
   `tools/make_mingw_def.py` genera un archivo de exportaciones MinGW:

   ```def
   LIBRARY binkw32
   EXPORTS
       _BinkOpen@8 = BinkOpen@8 @41
   ```

   El nombre público **permanece `_BinkOpen@8` con ordinal 41**; el nombre
   interno se refiere al símbolo COFF `_BinkOpen@8`. El `.def` original NO
   se altera, y el archivo generado solo se utiliza al compilar con MinGW.
   Se desactiva `--enable-stdcall-fixup` para evitar coincidencias ambiguas.

2. **`undefined reference to avcodec_*`, `avformat_*`, `swr_*`, `sws_*`.**
   La versión previa utilizaba `${AV_STATIC_LINK_LIBRARIES}`, variable que
   `FindPkgConfig` no define. La línea de enlace no incluía ningún `-lav*`.
   Ahora CMake usa `${AV_STATIC_LDFLAGS}`, que corresponde al resultado de
   `pkg-config --static --libs`, incluidos `Libs.private` y dependencias
   Win32. Se verifica la presencia de los cinco módulos y se muestran los
   flags al configurar y en el enlazado `--verbose` del workflow.
   MinGW agrupa las bibliotecas estáticas entre `--start-group` y
   `--end-group` para resolver dependencias circulares entre módulos.

## Verificaciones efectuadas localmente

- `ctest`: 7/7 pruebas correctas (Linux, sin FFmpeg).
- Test del generador de alias: 66/66 nombres y ordinales.
- Compilación de un archivo de objetos COFF x86 de **pruebas solamente** y
  enlace PE32 con `ld.lld`: 66/66 nombres y ordinales verificados mediante
  `tools/check_def.py`. **No es la DLL funcional del proyecto.**
- Comprobación del CMake generado con módulos pkg-config sintéticos:
  incorpora las cinco bibliotecas y las banderas `Libs.private`.

## Pendiente

Repetir GitHub Actions con el código actualizado, confirmar enlace exitoso
con el GCC MinGW de Ubuntu y los binarios FFmpeg compilados realmente, y
validar la decodificación con un fichero `.bik` legal. Las pruebas locales
no garantizan ejecución multimedia ni compatibilidad total con el juego.
