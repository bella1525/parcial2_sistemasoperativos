# PROCESADOR DE IMAGENES CONCURRENTE
Programa en C para procesamiento de imágenes PNG usando pthreads. Implementa operaciones matriciales con procesamiento paralelo para imágenes en escala de grises y RGB
## Nombres:
##### Juan Gonzalo de los rios 
##### Camilo Arbelaez
##### Isabella Bejarano
# Descripcion:
- Manejo de matrices 3D para representación de imágenes
- Concurrencia con POSIX threads (pthreads)
- Operaciones de convolución y transformaciones geométricas
- Interpolación bilineal para transformaciones de alta calidad
- Gestión robusta de memoria dinámica
## Funcionalidades
### Operacion base: 
- cargar y guardar imagenes PNG
- visualizar matricez de pixeles
- ajustar el brillo concurrentemente
### Operaciones Avanzadas (Concurrentes)
#### operacion que tenian que ser implementadas en el codigo base y ajustadas en el menu de opciones 
1. *Desenfoque Gaussiano*: Convolución con kernel Gaussiano configurable
2. *Redimensionar*: Escalado con interpolación bilineal
3. *Rotar*: Rotación por ángulo arbitrario con interpolación
4. *Detectar Bordes*: Operador Sobel para detección de bordes
### todas las operaciones usan 2 hilos en el procesamiento en paralelo 



 
