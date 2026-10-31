# Informe de análisis estático del archivo entregado

## Evidencia binaria

- Archivo aportado para investigación: `binkw32(1).dll` (no redistribuido en este proyecto)
- SHA-256: `1081a63d08c085f0aa027e0d2d3fa9eca3b33fa3403a2a6d333f277dbea91907`
- Tamaño: **193.024 bytes**
- Formato: PE32, x86 (`pei-i386`), Microsoft Windows DLL, GUI subsystem Windows 4.0
- Cabecera de enlazador: `7.10`; fecha PE: **2005-09-22 20:02:25** (campo PE, no necesariamente compilación real)
- Image base: `0x18000000`; Entry point RVA `0x17f0`; 13 secciones
- Tabla exportaciones: RVA `0x259a0`, 66 direcciones, base ordinal `1`, 66 nombres
- Todas las exportaciones observadas llevan decoración `_Nombre@N`, propia de stdcall x86.
- Imports observados de: `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `WINMM.dll`. Windows MM incluye waveOut y temporizadores. No se observó dependencia estática de `binkw32.dll` externa.
- Evidencia de subsistemas: nombres Bink audio y vídeo en exportaciones; llamadas a `waveOut*`, GDI, temporización `QueryPerformanceCounter`, `QueryPerformanceFrequency`, `GetTickCount` y `timeGetTime`, funciones de IO y de memoria.
- En la desensambladura en RVA `0xfa60` (`RADTimerRead`) se observan inicialización, consulta de QPC y rutas de reloj alternativo; devuelve tiempo relativo. La unidad **parece** ser milisegundos debido a una operación con factor `1000`, pero no ha sido verificada mediante ejecución diferencial.

## Lo que *no* demuestra el análisis

No hay PDB utilizable, símbolos internos fiables, trazas de ejecución originales, entradas multimedia reales ni pruebas diferenciales para esta DLL concreta. Las firmas y campos se infieren de decoraciones, documentación pública histórica y algunas instrucciones observadas; **no** existe prueba de layout completo de objetos `BINK` y `BINKBUFFER`. La compatibilidad real de tamaños, orden de operaciones, callbacks, errores, sincronización y escenarios de fallo sigue pendiente.

## ABI

El manifiesto `exports.json` contiene ordinal y nombre exacto para las 66 exportaciones. `src/binkw32.def` fue generado a partir de ese manifiesto. `tools/check_def.py` fue ejecutado también contra el PE32 compilado de esta implementación y verificó correspondencia exacta de nombre y ordinal.

## Codec independiente

La implementación real de FFmpeg para Bink (contenedor, decodificación de vídeo, y audio) es una posible vía de interoperabilidad, bajo licencia abierta. Este proyecto solo incluye un adaptador opcional a su **decoder de vídeo** y no contiene código de FFmpeg ni el algoritmo Bink propio completo. No se han demostrado ni la equivalencia de píxel ni la compatibilidad con todos los perfiles de Bink 1 de la biblioteca de 2005.

## Pruebas realizadas

- Compilación Linux de fuente C con GCC y CMake: **correcta**.
- CTest `core` y `export_manifest`: **2/2 correctos**, con `assert` habilitados.
- Compilación Windows 32 bits mediante clang+lld-link: **correcta**; PE32 generado.
- Exportaciones PE comparadas con los 66 nombres y ordinales del original: **66/66**.
- Imports de DLL Win32 corregidos para evitar nombres decorados incorrectos; comprobados con `objdump`.
- Ejecución real de Windows DLL: **no realizada**.
- Decodificación vídeo con FFmpeg activado: **no compilada ni probada**.
- Decodificación de audio/propagación en tiempo real: **no implementada**.
- Comparación diferencial original vs clon: **no realizada**.
