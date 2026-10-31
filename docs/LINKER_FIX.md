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

## Segundo arreglo: comprobador de exportaciones en GitHub Actions

El workflow enlazó correctamente `binkw32.dll`, pero el antiguo
`tools/check_def.py` informó de **0 exportaciones**. Esa cifra era el número
de entradas que coincidían con una expresión regular de la salida de
`objdump`, **no una lectura directa del directorio PE**. El registro no
permite determinar si el PE exportaba correctamente los 66 nombres o si el
linker había modificado su decoración. No se debe asumir ninguna de las dos.

Solución implementada:

- `tools/pe_exports.py` lee directamente el PE32, las secciones, el directorio
  de exportaciones, los punteros de nombres y la tabla de ordinales. Errores
  de PE truncado/dañado son errores reales, no una lista vacía.
- `tools/check_def.py` compara los nombres y ordinales exactos. Si difieren,
  muestra los primeros ordinales y nombres encontrados.
- `tools/choose_mingw_def.py` decide si el primer DLL ya está bien (`verified`)
  o si GNU ld le quitó (`double`) o añadió (`none`) un único `_` a cada nombre.
  Cualquier otra diferencia interrumpe el workflow.
- `BINK_MINGW_PUBLIC_NAME_PREFIX` permite a CMake generar otra variante del
  `.def` y enlazar nuevamente. **La verificación estricta vuelve a ejecutarse
  después de reconstruir**; no se modifica el manifiesto original.
- `tests/test_pe_exports.py` comprueba archivos PE32 sintéticos, ordinales no
  consecutivos, detección de errores y selección de la corrección.

**Verificado localmente:** ocho pruebas nativas y comparación directa PE32
contra la DLL de referencia y contra el binario anterior (66/66). **No
verificado:** el resultado de la recompilación real con FFmpeg/MinGW en el
runner remoto; el workflow debe confirmarlo antes de publicar su artefacto.

## Tercer arreglo: verificación de importaciones estáticas en CI

Después de enlazar `binkw32.dll`, el verificador de PE32 aceptó sus 66
exportaciones, pero `tools/check_win32_dll.py` se detuvo con
`re.error: missing ), unterminated subpattern at position 0` al clasificar
los nombres `DLL Name:` obtenidos de `objdump -p`. La expresión regular
contenía un grupo `(?:` que no se cerraba.

El verificador ahora delega en `tools/win32_imports.py`, que combina un
conjunto explícito de DLL del sistema Windows con una expresión regular
pequeña y válida para los contratos `api-ms-win-*` / `ext-ms-*`. Se sigue
rechazando la importación dinámica de `avcodec-*.dll`, `avformat-*.dll` y
otras dependencias ajenas a Windows. La validación **no se elimina ni
se omite** para pasar el workflow.

`tests/test_win32_imports.py` comprueba la clasificación y ejecuta el
verificador completo sobre un PE32 sintético con 66 exportaciones,
simulando la salida de `objdump`. Una segunda prueba asegura que una
importación dinámica de FFmpeg se detecta como error. No se requiere ni
se distribuye la DLL propietaria para ejecutar estas pruebas.

**Pruebas efectuadas localmente:** 9/9 `ctest`; ejecución completa del
verificador contra la DLL original proporcionada en la conversación,
con 66 exportaciones y 4 importaciones del sistema reconocidas.
**Pendiente:** ejecutar otra vez GitHub Actions para comprobar el PE32
nuevo construido con FFmpeg y su lista real de importaciones.
