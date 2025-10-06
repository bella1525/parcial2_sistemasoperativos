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
## Requisitos
- Compilador GCC o Clang
- Librería pthread (incluida en Linux/macOS, MinGW en Windows)
- Librería matemática (flag -lm)
## Compilación
bash
# Compilación básica
gcc -o img procesador_imagenes/base.c -pthread -lm
# Con advertencias (recomendado)
gcc -o img procesador_imagenes/base.c -pthread -lm -Wall -Wextra
# Optimizado
gcc -o img procesador_imagenes/base.c -pthread -lm -O2
# *Windows (MinGW/MSYS2):*
bash
gcc -o img.exe procesador_imagenes/base.c -pthread -lm
## Ejecución
bash
# Modo interactivo
./img
# Cargar imagen al inicio
./img procesador_imagenes/carro.png
## Menú Interactivo
1. Cargar imagen PNG (la imagen al guardarla tiene que estar en este formato png)
2. Mostrar matriz de píxeles (mostrara la matriz de pixeles de la imagen que es cargada)
3. Guardar como PNG (guardar la imagen despues de cada modificacion en el menu, esta sera guardada en la carperta)
4. Ajustar brillo (+/- valor) concurrentemente (ajuste del brillos de la imagen)
5. Aplicar desenfoque Gaussiano (convolución)
6. Redimensionar imagen (escalar)
7. Rotar imagen 
8. Detectar bordes (Sobel)
9. Salir
## Ejemplos de uso 
https://youtu.be/GscDY0mI2A8  (video de como se hace el uso del programa)
### Aplicar desenfoque y guardar
bash
./img procesador_imagenes/emoji.png
# Opción: 5 → Tamaño kernel: 5, Sigma: 1.5
# Opción: 3 → Nombre: emoji_blur.png
### Redimensionar imagen
bash
./img procesador_imagenes/carro.png
# Opción: 6 → Nuevo ancho: 400, Nuevo alto: 300
# Opción: 3 → Nombre: carro_resized.png
### Rotar y detectar bordes
bash
./img procesador_imagenes/troste.png
# Opción: 7 → Ángulo: 45
# Opción: 8 (detectar bordes Sobel)
# Opción: 3 → Nombre: troste_procesado.png
### Pipeline completo
bash
./img procesador_imagenes/emoji.png
# 1. Opción: 6 → Escalar a 200x200
# 2. Opción: 5 → Desenfoque (kernel=3, sigma=1.0)
# 3. Opción: 4 → Ajustar brillo (+30)
# 4. Opción: 3 → Guardar resultado
## Detalles tecnicos 
### Concurrencia:
- División de trabajo por filas entre hilos
- Sincronización con pthread_join()
- Sin race conditions (lectura compartida, escritura independiente)














 
