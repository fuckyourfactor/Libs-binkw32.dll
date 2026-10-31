# Matriz de compatibilidad — muestra original 2005

**Leyenda:** `UNITARIO` = funcionamiento limitado probado mediante casos sintéticos, **NO equivale a paridad**; `PARCIAL` = comportamiento limitado/reconstruido, sin pruebas diferenciales; `PENDIENTE` = existe una ruta real de código de vídeo opcional, pero no se compiló/validó con material Bink; `NO` = explícitamente no implementado. Las 66 exportaciones existen en el PE pero muchas no tienen implementación equivalente.

| Ordinal | Exportación exacta | Estado | Evidencia / diferencia |
|---:|---|---|---|
| 1 | `_BinkBufferBlit@12` | PARCIAL | Windows GDI: blit de fotograma entero; rectángulos sin aplicar; no probado en Windows |
| 2 | `_BinkBufferCheckWinPos@12` | PARCIAL | Devuelve desplazamientos almacenados; no consulta posición de ventana |
| 3 | `_BinkBufferClear@8` | UNITARIO | Rellena memoria BGRA; probado con buffer pequeño |
| 4 | `_BinkBufferClose@4` | UNITARIO | Libera buffer propio; prueba de integración básica |
| 5 | `_BinkBufferGetDescription@4` | PARCIAL | Describe backend nuevo; cadena no equivalente |
| 6 | `_BinkBufferGetError@0` | PARCIAL | Informe de error de backend propio; contenido no equivalente |
| 7 | `_BinkBufferLock@4` | PARCIAL | Da acceso a memoria propia; bloqueo real no implementado |
| 8 | `_BinkBufferOpen@16` | UNITARIO | Abre buffer BGRA CPU para modos Auto/DIB; demás modos faltan |
| 9 | `_BinkBufferSetDirectDraw@8` | NO | Rechazo explícito de DirectDraw |
| 10 | `_BinkBufferSetHWND@8` | PARCIAL | Actualiza handle de ventana; no consulta compatibilidad |
| 11 | `_BinkBufferSetOffset@12` | UNITARIO | Actualiza coordenadas de destino |
| 12 | `_BinkBufferSetResolution@12` | NO | No cambia resolución de pantalla |
| 13 | `_BinkBufferSetScale@12` | UNITARIO | Actualiza dimensiones de escalado de buffer |
| 14 | `_BinkBufferUnlock@4` | PARCIAL | No hay política de bloqueo de superficies reales |
| 15 | `_BinkCheckCursor@20` | NO | Cursor software de DirectDraw no implementado |
| 16 | `_BinkClose@4` | UNITARIO | Libera fuentes, índice, imagen y contexto |
| 17 | `_BinkCloseTrack@4` | PARCIAL | Libera objeto de pista; no dispositivo de audio |
| 18 | `_BinkControlBackgroundIO@8` | NO | No se implementa hilo de IO |
| 19 | `_BinkControlPlatformFeatures@8` | NO | Máscaras de CPU sin implementación |
| 20 | `_BinkCopyToBuffer@28` | PENDIENTE | Copia BGRA decodificado a superficies RGB 24/32; sin prueba con vídeo real |
| 21 | `_BinkCopyToBufferRect@44` | PENDIENTE | Copia de rectángulos RGB 24/32; sin prueba sobre vídeo real |
| 22 | `_BinkDDSurfaceType@4` | NO | Introspección de superficies DirectDraw no implementada |
| 23 | `_BinkDX8SurfaceType@4` | NO | Mapeo Direct3D8 no implementado |
| 24 | `_BinkDX9SurfaceType@4` | NO | Mapeo Direct3D9 no implementado |
| 25 | `_BinkDoFrame@4` | PENDIENTE | Backend FFmpeg vídeo Bink1 opcional no compilado ni probado; compilación distribuida falla explícitamente |
| 26 | `_BinkGetError@0` | UNITARIO | Cadena de error propia, mensajes diferentes del original |
| 27 | `_BinkGetFrameBuffersInfo@8` | PARCIAL | Propuesta de formato YUV420; no verificada contra 2005 |
| 28 | `_BinkGetKeyFrame@12` | UNITARIO | Índice de keyframes probado con contenedores sintéticos |
| 29 | `_BinkGetPalette@4` | NO | No disponible en backend RGB |
| 30 | `_BinkGetRealtime@12` | PARCIAL | Métricas básicas, no incorpora contadores históricos ni IO |
| 31 | `_BinkGetRects@8` | PARCIAL | Rectángulo completo en fotogramas decodificados |
| 32 | `_BinkGetSummary@8` | UNITARIO | Dimensiones, fps, duración, tasa de datos; resto de métricas en cero |
| 33 | `_BinkGetTrackData@8` | NO | No decodifica audio Bink a PCM |
| 34 | `_BinkGetTrackID@8` | UNITARIO | Obtiene identificador de pista del contenedor |
| 35 | `_BinkGetTrackMaxSize@8` | UNITARIO | Obtiene tamaño máximo de bloque del contenedor |
| 36 | `_BinkGetTrackType@8` | UNITARIO | Obtiene banderas del audio del contenedor |
| 37 | `_BinkGoto@12` | UNITARIO | Cambia posición lógica; FFmpeg redecodifica al retroceder |
| 38 | `_BinkIsSoftwareCursor@8` | NO | No hay detección de cursor software |
| 39 | `_BinkLogoAddress@0` | NO | No incluye material gráfico propietario |
| 40 | `_BinkNextFrame@4` | UNITARIO | Avance/ciclo de fotogramas y temporizador |
| 41 | `_BinkOpen@8` | UNITARIO | Lee contenedor Bink1 válido, indexa y prepara estado; decodificación opcional |
| 42 | `_BinkOpenDirectSound@4` | NO | No dispone de salida DirectSound |
| 43 | `_BinkOpenMiles@4` | NO | No dispone de adaptador Miles |
| 44 | `_BinkOpenTrack@8` | PARCIAL | Crea descriptor de frecuencia/canales; no decodifica audio |
| 45 | `_BinkOpenWaveOut@4` | NO | No dispone de salida WaveOut |
| 46 | `_BinkPause@8` | UNITARIO | Estado de pausa de reloj; sin audio conectado |
| 47 | `_BinkRegisterFrameBuffers@8` | PARCIAL | Registra puntero; no lo usa el decodificador |
| 48 | `_BinkRestoreCursor@4` | NO | Restauración de cursor no disponible |
| 49 | `_BinkService@4` | NO | Informa que no hay servicio de audio/IO |
| 50 | `_BinkSetError@4` | UNITARIO | Guarda cadena de error, parcial por textos diferentes |
| 51 | `_BinkSetFrameRate@8` | PARCIAL | Aplica override al abrir con bandera BINKFRAMERATE |
| 52 | `_BinkSetIO@4` | NO | Callbacks IO no implementados |
| 53 | `_BinkSetIOSize@4` | NO | Solo registra dato, sin efecto real; se informa del límite |
| 54 | `_BinkSetMemory@8` | PARCIAL | Callback de asignación solo para píxeles BGRA; no para todas las estructuras |
| 55 | `_BinkSetMixBinVolumes@20` | NO | No hay mezclador |
| 56 | `_BinkSetMixBins@16` | NO | No hay mezclador |
| 57 | `_BinkSetPan@12` | NO | No hay sistema de audio |
| 58 | `_BinkSetSimulate@4` | NO | Sin simulación de retrasos de IO |
| 59 | `_BinkSetSoundOnOff@8` | PARCIAL | Registra estado sin salida sonora |
| 60 | `_BinkSetSoundSystem@8` | NO | No integra controlador de audio |
| 61 | `_BinkSetSoundTrack@8` | NO | No se vincula lista a audio reproducible |
| 62 | `_BinkSetVideoOnOff@8` | PARCIAL | Controla si se pide fotograma al decodificador |
| 63 | `_BinkSetVolume@12` | NO | Volumen no conectado a audio |
| 64 | `_BinkShouldSkip@4` | PARCIAL | Heurística de latencia, sin sincronización audio |
| 65 | `_BinkWait@4` | PARCIAL | Reloj aproximado sin sincronización sonora |
| 66 | `_RADTimerRead@0` | PARCIAL | Contador de milisegundos desde primera llamada; QPC original difiere |
