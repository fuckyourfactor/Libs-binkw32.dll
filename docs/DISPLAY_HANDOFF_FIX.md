# rFactor: decodificación correcta, presentación bloqueada

## Evidencia del LOG.txt del juego

- `BinkOpen OK 600x450 frames=172 audio_tracks=1`: el contenedor se abre.
- `BinkDoFrame decoded frame=1 ... rgb_sample=6A792000`: se obtiene un fotograma
  con contenido no nulo; no prueba por sí solo que se haya presentado.
- `BinkCopyToBufferRect no frame ... frame=2 decoded=1`: se rechaza la copia del
  fotograma 1 tras avanzar el contador público.
- `BinkGetRects no decoded frame current=2 decoded=1 flags=0x00000000`:
  el rechazo afecta también a la obtención de rectángulos.
- El registro contiene **2.410** rechazos `BinkCopyToBufferRect no frame`, **0**
  mensajes `BinkCopyToBufferRect OK` y **25** mensajes `BinkGetRects`.
- La cola de audio `WaveOut` también se llena en ocasiones, pero el bloqueo
  de vídeo está identificado independientemente de ese aviso.

## Secuencia de la aplicación

```text
BinkDoFrame(1)     -> rgb contiene el fotograma 1, frame_decoded=1
BinkNextFrame()    -> BINK.FrameNum=2; rgb TODAVÍA contiene el fotograma 1
BinkGetRects()     -> debe describir el fotograma 1
BinkCopyToBufferRect() -> debe copiar el fotograma 1 al búfer de rFactor
```

La comparación antigua `frame_decoded != b->FrameNum` producía el rechazo
inevitable `1 != 2`. El mismo error ocurría para todos los fotogramas
posteriores.

## Cambios del código

- `src/bink_api.c`: `has_presentable_frame()` determina disponibilidad a partir
  del búfer RGB y del identificador del **último fotograma decodificado**.
  `FrameNum` ya no se utiliza como señal de invalidez en `GetRects` ni `Copy`.
- `BinkNextFrame()` preserva la imagen que el motor aún debe copiar.
- `BinkGoto()` borra `frame_decoded` y `NumRects` para evitar usar una imagen
  obsoleta después de un salto de reproducción.
- `src/private.h`: contador de avisos de copia fallida, para que un fallo
  repetido no genere miles de líneas de log.
- `tests/test_video_api.c`: prueba exacta de dos copias tras `NextFrame`,
  comprobación de rectángulos y invalidación al ejecutar `Goto`.

## Validación y límites

- `ctest` en Linux sin FFmpeg: **10/10**.
- `ctest` con `-fsanitize=undefined`: **10/10**.
- Clang/LLD PE32 de 32 bits sin FFmpeg: **66/66** exportaciones originales.
- No se ha ejecutado rFactor en este contenedor; no hay vídeo Bink real
  suministrado para comprobar los formatos de destino y FFmpeg.
- El workflow GitHub Actions existente compila FFmpeg estáticamente y debe
  usarse para obtener la DLL que se instala en el juego.

## Próximo registro de comprobación

```powershell
Remove-Item "$env:TEMP\bink-open.log" -ErrorAction SilentlyContinue
$env:BINK_OPEN_TRACE = "$env:TEMP\bink-open.log"
.\rFactor.exe
Select-String -Path "$env:TEMP\bink-open.log" -Pattern 'BinkGetRects|BinkCopyToBufferRect' | Select-Object -First 40
```

La nueva versión debería dejar de emitir repetidamente `no frame ...
next=N decoded=N-1` y comenzar a emitir `BinkCopyToBufferRect OK`.
Si aparece `FAILED`, el log aporta `flags`, `pitch`, altura y rectángulo
para investigar el formato de destino; no se debe anunciar éxito antes
 de obtener esa confirmación en el juego.
