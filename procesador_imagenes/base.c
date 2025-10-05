// Programa de procesamiento de imágenes en C para principiantes en Linux.
// QUÉ: Procesa imágenes PNG (escala de grises o RGB) usando matrices, con soporte
// para carga, visualización, guardado y ajuste de brillo concurrente.
// CÓMO: Usa stb_image.h para cargar PNG y stb_image_write.h para guardar PNG,
// con hilos POSIX (pthread) para el procesamiento paralelo del brillo.
// POR QUÉ: Diseñado para enseñar manejo de matrices, concurrencia y gestión de
// memoria en C, manteniendo simplicidad y robustez para principiantes.
// Dependencias: Descarga stb_image.h y stb_image_write.h desde
// https://github.com/nothings/stb
//   wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
//   wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
//
// Compilar: gcc -o img img_base.c -pthread -lm
// Ejecutar: ./img [ruta_imagen.png]

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

// QUÉ: Incluir bibliotecas stb para cargar y guardar imágenes PNG.
// CÓMO: stb_image.h lee PNG/JPG a memoria; stb_image_write.h escribe PNG.
// POR QUÉ: Son bibliotecas de un solo archivo, simples y sin dependencias externas.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// QUÉ: Estructura para almacenar la imagen (ancho, alto, canales, píxeles).
// CÓMO: Usa matriz 3D para píxeles (alto x ancho x canales), donde canales es
// 1 (grises) o 3 (RGB). Píxeles son unsigned char (0-255).
// POR QUÉ: Permite manejar tanto grises como color, con memoria dinámica para
// flexibilidad y evitar desperdicio.
typedef struct {
    int ancho;           // Ancho de la imagen en píxeles
    int alto;            // Alto de la imagen en píxeles
    int canales;         // 1 (escala de grises) o 3 (RGB)
    unsigned char*** pixeles; // Matriz 3D: [alto][ancho][canales]
} ImagenInfo;

// =====================================================================
// FUNCIONES AUXILIARES DE MANEJO DE MEMORIA
// =====================================================================

// QUÉ: Asigna memoria para una matriz 3D de píxeles (alto x ancho x canales).
// CÓMO: Asigna en tres niveles: filas, columnas y canales por píxel. Si falla 
// en cualquier nivel, libera toda la memoria ya asignada y retorna NULL.
// POR QUÉ: Centraliza la asignación de matrices 3D con manejo robusto de errores,
// evitando fugas de memoria y duplicación de código en todas las funciones.
unsigned char*** asignarMatriz3D(int alto, int ancho, int canales) {
    // Validar parámetros
    if (alto <= 0 || ancho <= 0 || canales <= 0) {
        fprintf(stderr, "Error: Parámetros inválidos para asignarMatriz3D (alto=%d, ancho=%d, canales=%d)\n",
                alto, ancho, canales);
        return NULL;
    }

    // Nivel 1: Asignar arreglo de filas
    unsigned char*** matriz = malloc(alto * sizeof(unsigned char**));
    if (!matriz) {
        fprintf(stderr, "Error de memoria: No se pudo asignar arreglo de filas\n");
        return NULL;
    }

    // Nivel 2: Asignar arreglo de columnas para cada fila
    for (int y = 0; y < alto; y++) {
        matriz[y] = malloc(ancho * sizeof(unsigned char*));
        if (!matriz[y]) {
            fprintf(stderr, "Error de memoria: No se pudo asignar columnas en fila %d\n", y);
            // Liberar filas ya asignadas
            for (int yy = 0; yy < y; yy++) {
                for (int x = 0; x < ancho; x++) {
                    free(matriz[yy][x]);
                }
                free(matriz[yy]);
            }
            free(matriz);
            return NULL;
        }

        // Nivel 3: Asignar canales para cada píxel de la fila
        for (int x = 0; x < ancho; x++) {
            matriz[y][x] = malloc(canales * sizeof(unsigned char));
            if (!matriz[y][x]) {
                fprintf(stderr, "Error de memoria: No se pudo asignar canales en [%d][%d]\n", y, x);
                // Liberar píxeles de esta fila ya asignados
                for (int xx = 0; xx < x; xx++) {
                    free(matriz[y][xx]);
                }
                free(matriz[y]);
                // Liberar filas anteriores completamente
                for (int yy = 0; yy < y; yy++) {
                    for (int xx = 0; xx < ancho; xx++) {
                        free(matriz[yy][xx]);
                    }
                    free(matriz[yy]);
                }
                free(matriz);
                return NULL;
            }
        }
    }

    return matriz;
}

// QUÉ: Libera la memoria de una matriz 3D de píxeles.
// CÓMO: Libera en orden inverso a la asignación: primero canales (píxeles), 
// luego columnas (filas), finalmente el arreglo principal.
// POR QUÉ: Evita fugas de memoria liberando todos los niveles de la matriz 3D
// correctamente, con verificación de puntero nulo para robustez.
void liberarMatriz3D(unsigned char*** matriz, int alto, int ancho) {
    if (!matriz) {
        return; // Seguro ante punteros nulos
    }

    for (int y = 0; y < alto; y++) {
        if (matriz[y]) {
            for (int x = 0; x < ancho; x++) {
                free(matriz[y][x]); // Liberar canales de cada píxel
            }
            free(matriz[y]); // Liberar fila (columnas)
        }
    }
    free(matriz); // Liberar arreglo principal (filas)
}

// QUÉ: Crea una copia completa (clon) de una matriz 3D de píxeles.
// CÓMO: Asigna nueva matriz con asignarMatriz3D(), luego copia píxel por píxel
// todos los canales desde la matriz origen.
// POR QUÉ: Necesario para operaciones que requieren preservar la imagen original
// mientras crean una versión modificada (filtros, transformaciones).
unsigned char*** clonarMatriz3D(unsigned char*** origen, int alto, int ancho, int canales) {
    // Validar parámetros
    if (!origen) {
        fprintf(stderr, "Error: Matriz origen es NULL en clonarMatriz3D\n");
        return NULL;
    }
    if (alto <= 0 || ancho <= 0 || canales <= 0) {
        fprintf(stderr, "Error: Parámetros inválidos para clonarMatriz3D (alto=%d, ancho=%d, canales=%d)\n",
                alto, ancho, canales);
        return NULL;
    }

    // Asignar nueva matriz
    unsigned char*** clon = asignarMatriz3D(alto, ancho, canales);
    if (!clon) {
        fprintf(stderr, "Error: No se pudo asignar memoria para clonar matriz\n");
        return NULL;
    }

    // Copiar píxel por píxel
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            for (int c = 0; c < canales; c++) {
                clon[y][x][c] = origen[y][x][c];
            }
        }
    }

    return clon;
}

// =====================================================================
// FUNCIONES AUXILIARES DE INTERPOLACIÓN
// =====================================================================

// QUÉ: Calcula el valor interpolado de un píxel en coordenadas fraccionarias
// usando interpolación bilineal.
// CÓMO: Obtiene los 4 píxeles vecinos (esquinas del rectángulo que contiene el
// punto), calcula pesos basados en distancias fraccionarias, y promedia los
// valores con la fórmula bilineal: I = (1-a)(1-b)I00 + a(1-b)I10 + (1-a)bI01 + abI11.
// Aplica clamping en los bordes para evitar accesos fuera de límites.
// POR QUÉ: Necesario para operaciones de transformación geométrica (rotación,
// escalado) que mapean píxeles a coordenadas no enteras, produciendo resultados
// suaves sin aliasing.
unsigned char interpolacionBilineal(unsigned char*** img, float x, float y, int c, 
                                   int ancho, int alto) {
    // Obtener coordenadas enteras de las 4 esquinas del rectángulo
    int x0 = (int)floor(x);  // Esquina superior izquierda X
    int y0 = (int)floor(y);  // Esquina superior izquierda Y
    int x1 = x0 + 1;         // Esquina superior derecha X
    int y1 = y0 + 1;         // Esquina inferior izquierda Y
    
    // Clamp coordenadas a límites válidos de la imagen
    // Esto maneja píxeles en los bordes replicando el píxel más cercano
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= ancho) x1 = ancho - 1;
    if (y1 >= alto) y1 = alto - 1;
    
    // Calcular partes fraccionarias (pesos de interpolación)
    // a = distancia horizontal desde x0 (0.0 = en x0, 1.0 = en x1)
    // b = distancia vertical desde y0 (0.0 = en y0, 1.0 = en y1)
    float a = x - x0;
    float b = y - y0;
    
    // Obtener valores de los 4 píxeles vecinos
    // v00 = esquina superior izquierda
    // v10 = esquina superior derecha
    // v01 = esquina inferior izquierda
    // v11 = esquina inferior derecha
    float v00 = img[y0][x0][c];
    float v10 = img[y0][x1][c];
    float v01 = img[y1][x0][c];
    float v11 = img[y1][x1][c];
    
    // Aplicar fórmula de interpolación bilineal
    // Interpola primero en X (horizontalmente) para las dos filas,
    // luego interpola en Y (verticalmente) entre los resultados
    float resultado = (1.0f - a) * (1.0f - b) * v00  // Peso esquina superior izquierda
                    + a * (1.0f - b) * v10            // Peso esquina superior derecha
                    + (1.0f - a) * b * v01            // Peso esquina inferior izquierda
                    + a * b * v11;                    // Peso esquina inferior derecha
    
    // Redondear y convertir a unsigned char
    // Suma 0.5 para redondeo correcto antes del truncado
    return (unsigned char)(resultado + 0.5f);
}

// =====================================================================
// FUNCIONES AUXILIARES DE CONVOLUCIÓN
// =====================================================================

// QUÉ: Genera un kernel Gaussiano 2D para operaciones de desenfoque (blur).
// CÓMO: Asigna matriz 2D de floats, aplica fórmula Gaussiana G(x,y) = 
// (1/(2πσ²)) * e^(-(x²+y²)/(2σ²)) para cada elemento, luego normaliza
// dividiendo por la suma total para que todos los pesos sumen 1.0.
// POR QUÉ: El kernel Gaussiano produce desenfoque natural preservando bordes
// mejor que un simple promedio. La normalización garantiza que el brillo de
// la imagen se mantenga constante después de la convolución.
float** generarKernelGaussiano(int tamKernel, float sigma) {
    // Validar que el tamaño del kernel sea impar
    if (tamKernel <= 0 || tamKernel % 2 == 0) {
        fprintf(stderr, "Error: El tamaño del kernel debe ser impar y positivo (recibido: %d)\n", 
                tamKernel);
        return NULL;
    }
    
    // Validar que sigma sea positivo
    if (sigma <= 0.0f) {
        fprintf(stderr, "Error: Sigma debe ser positivo (recibido: %.2f)\n", sigma);
        return NULL;
    }
    
    // Asignar memoria para el kernel (matriz 2D de floats)
    float** kernel = malloc(tamKernel * sizeof(float*));
    if (!kernel) {
        fprintf(stderr, "Error de memoria: No se pudo asignar kernel\n");
        return NULL;
    }
    
    for (int i = 0; i < tamKernel; i++) {
        kernel[i] = malloc(tamKernel * sizeof(float));
        if (!kernel[i]) {
            fprintf(stderr, "Error de memoria: No se pudo asignar fila %d del kernel\n", i);
            // Liberar filas ya asignadas
            for (int j = 0; j < i; j++) {
                free(kernel[j]);
            }
            free(kernel);
            return NULL;
        }
    }
    
    // Calcular el offset desde el centro del kernel
    int offset = tamKernel / 2;
    
    // Calcular constante de la fórmula Gaussiana: 1 / (2πσ²)
    float constante = 1.0f / (2.0f * M_PI * sigma * sigma);
    
    // Calcular denominador del exponente: 2σ²
    float denominador = 2.0f * sigma * sigma;
    
    float suma = 0.0f; // Acumulador para normalización
    
    // Generar valores del kernel usando la fórmula Gaussiana
    for (int ky = -offset; ky <= offset; ky++) {
        for (int kx = -offset; kx <= offset; kx++) {
            // Calcular G(x,y) = (1/(2πσ²)) * e^(-(x²+y²)/(2σ²))
            float exponente = -(kx * kx + ky * ky) / denominador;
            float valor = constante * exp(exponente);
            
            // Almacenar en la matriz (convertir coordenadas relativas a índices)
            kernel[ky + offset][kx + offset] = valor;
            suma += valor;
        }
    }
    
    // Normalizar el kernel para que la suma sea 1.0
    // Esto garantiza que el brillo promedio de la imagen no cambie
    if (suma > 0.0f) {
        for (int i = 0; i < tamKernel; i++) {
            for (int j = 0; j < tamKernel; j++) {
                kernel[i][j] /= suma;
            }
        }
    }
    
    return kernel;
}

// QUÉ: Liberar memoria asignada para la imagen.
// CÓMO: Libera cada fila y canal de la matriz 3D, luego el arreglo de filas y
// reinicia la estructura.
// POR QUÉ: Evita fugas de memoria, esencial en C para manejar recursos manualmente.
void liberarImagen(ImagenInfo* info) {
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]); // Liberar canales por píxel
            }
            free(info->pixeles[y]); // Liberar fila
        }
        free(info->pixeles); // Liberar arreglo de filas
        info->pixeles = NULL;
    }
    info->ancho = 0;
    info->alto = 0;
    info->canales = 0;
}

// QUÉ: Cargar una imagen PNG desde un archivo.
// CÓMO: Usa stbi_load para leer el archivo, detecta canales (1 o 3), y convierte
// los datos a una matriz 3D (alto x ancho x canales).
// POR QUÉ: La matriz 3D es intuitiva para principiantes y permite procesar
// píxeles y canales individualmente.
int cargarImagen(const char* ruta, ImagenInfo* info) {
    int canales;
    // QUÉ: Cargar imagen con formato original (0 canales = usar formato nativo).
    // CÓMO: stbi_load lee el archivo y llena ancho, alto y canales.
    // POR QUÉ: Respetar el formato original asegura que grises o RGB se mantengan.
    unsigned char* datos = stbi_load(ruta, &info->ancho, &info->alto, &canales, 0);
    if (!datos) {
        fprintf(stderr, "Error al cargar imagen: %s\n", ruta);
        return 0;
    }
    info->canales = (canales == 1 || canales == 3) ? canales : 1; // Forzar 1 o 3

    // QUÉ: Asignar memoria para matriz 3D.
    // CÓMO: Asignar alto filas, luego ancho columnas por fila, luego canales por píxel.
    // POR QUÉ: Estructura clara y flexible para grises (1 canal) o RGB (3 canales).
    info->pixeles = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!info->pixeles) {
        fprintf(stderr, "Error de memoria al asignar filas\n");
        stbi_image_free(datos);
        return 0;
    }
    for (int y = 0; y < info->alto; y++) {
        info->pixeles[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!info->pixeles[y]) {
            fprintf(stderr, "Error de memoria al asignar columnas\n");
            liberarImagen(info);
            stbi_image_free(datos);
            return 0;
        }
        for (int x = 0; x < info->ancho; x++) {
            info->pixeles[y][x] = (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!info->pixeles[y][x]) {
                fprintf(stderr, "Error de memoria al asignar canales\n");
                liberarImagen(info);
                stbi_image_free(datos);
                return 0;
            }
            // Copiar píxeles a matriz 3D
            for (int c = 0; c < info->canales; c++) {
                info->pixeles[y][x][c] = datos[(y * info->ancho + x) * info->canales + c];
            }
        }
    }

    stbi_image_free(datos); // Liberar buffer de stb
    printf("Imagen cargada: %dx%d, %d canales (%s)\n", info->ancho, info->alto,
           info->canales, info->canales == 1 ? "grises" : "RGB");
    return 1;
}

// QUÉ: Mostrar la matriz de píxeles (primeras 10 filas).
// CÓMO: Imprime los valores de los píxeles, agrupando canales por píxel (grises o RGB).
// POR QUÉ: Ayuda a visualizar la matriz para entender la estructura de datos.
void mostrarMatriz(const ImagenInfo* info) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    printf("Matriz de la imagen (primeras 10 filas):\n");
    for (int y = 0; y < info->alto && y < 10; y++) {
        for (int x = 0; x < info->ancho; x++) {
            if (info->canales == 1) {
                printf("%3u ", info->pixeles[y][x][0]); // Escala de grises
            } else {
                printf("(%3u,%3u,%3u) ", info->pixeles[y][x][0], info->pixeles[y][x][1],
                       info->pixeles[y][x][2]); // RGB
            }
        }
        printf("\n");
    }
    if (info->alto > 10) {
        printf("... (más filas)\n");
    }
}

// QUÉ: Guardar la matriz como PNG (grises o RGB).
// CÓMO: Aplana la matriz 3D a 1D y usa stbi_write_png con el número de canales correcto.
// POR QUÉ: Respeta el formato original (grises o RGB) para consistencia.
int guardarPNG(const ImagenInfo* info, const char* rutaSalida) {
    if (!info->pixeles) {
        fprintf(stderr, "No hay imagen para guardar.\n");
        return 0;
    }

    // QUÉ: Aplanar matriz 3D a 1D para stb.
    // CÓMO: Copia píxeles en orden [y][x][c] a un arreglo plano.
    // POR QUÉ: stb_write_png requiere datos contiguos.
    unsigned char* datos1D = (unsigned char*)malloc(info->ancho * info->alto * info->canales);
    if (!datos1D) {
        fprintf(stderr, "Error de memoria al aplanar imagen\n");
        return 0;
    }
    for (int y = 0; y < info->alto; y++) {
        for (int x = 0; x < info->ancho; x++) {
            for (int c = 0; c < info->canales; c++) {
                datos1D[(y * info->ancho + x) * info->canales + c] = info->pixeles[y][x][c];
            }
        }
    }

    // QUÉ: Guardar como PNG.
    // CÓMO: Usa stbi_write_png con los canales de la imagen original.
    // POR QUÉ: Mantiene el formato (grises o RGB) de la entrada.
    int resultado = stbi_write_png(rutaSalida, info->ancho, info->alto, info->canales,
                                   datos1D, info->ancho * info->canales);
    free(datos1D);
    if (resultado) {
        printf("Imagen guardada en: %s (%s)\n", rutaSalida,
               info->canales == 1 ? "grises" : "RGB");
        return 1;
    } else {
        fprintf(stderr, "Error al guardar PNG: %s\n", rutaSalida);
        return 0;
    }
}

// QUÉ: Estructura para pasar datos al hilo de ajuste de brillo.
// CÓMO: Contiene matriz, rango de filas, ancho, canales y delta de brillo.
// POR QUÉ: Los hilos necesitan datos específicos para procesar en paralelo.
typedef struct {
    unsigned char*** pixeles;
    int inicio;
    int fin;
    int ancho;
    int canales;
    int delta;
} BrilloArgs;

// QUÉ: Ajustar brillo en un rango de filas (para hilos).
// CÓMO: Suma delta a cada canal de cada píxel, con clamp entre 0-255.
// POR QUÉ: Procesa píxeles en paralelo para demostrar concurrencia.
void* ajustarBrilloHilo(void* args) {
    BrilloArgs* bArgs = (BrilloArgs*)args;
    for (int y = bArgs->inicio; y < bArgs->fin; y++) {
        for (int x = 0; x < bArgs->ancho; x++) {
            for (int c = 0; c < bArgs->canales; c++) {
                int nuevoValor = bArgs->pixeles[y][x][c] + bArgs->delta;
                bArgs->pixeles[y][x][c] = (unsigned char)(nuevoValor < 0 ? 0 :
                                                          (nuevoValor > 255 ? 255 : nuevoValor));
            }
        }
    }
    return NULL;
}

// QUÉ: Ajustar brillo de la imagen usando múltiples hilos.
// CÓMO: Divide las filas entre 2 hilos, pasa argumentos y espera con join.
// POR QUÉ: Usa concurrencia para acelerar el procesamiento y enseñar hilos.
void ajustarBrilloConcurrente(ImagenInfo* info, int delta) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }

    const int numHilos = 2; // QUÉ: Número fijo de hilos para simplicidad.
    pthread_t hilos[numHilos];
    BrilloArgs args[numHilos];
    int filasPorHilo = (int)ceil((double)info->alto / numHilos);

    // QUÉ: Configurar y lanzar hilos.
    // CÓMO: Asigna rangos de filas a cada hilo y pasa datos.
    // POR QUÉ: Divide el trabajo para procesar en paralelo.
    for (int i = 0; i < numHilos; i++) {
        args[i].pixeles = info->pixeles;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i + 1) * filasPorHilo < info->alto ? (i + 1) * filasPorHilo : info->alto;
        args[i].ancho = info->ancho;
        args[i].canales = info->canales;
        args[i].delta = delta;
        if (pthread_create(&hilos[i], NULL, ajustarBrilloHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            return;
        }
    }

    // QUÉ: Esperar a que los hilos terminen.
    // CÓMO: Usa pthread_join para sincronizar.
    // POR QUÉ: Garantiza que todos los píxeles se procesen antes de continuar.
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    printf("Brillo ajustado concurrentemente con %d hilos (%s).\n", numHilos,
           info->canales == 1 ? "grises" : "RGB");
}

// QUÉ: Mostrar el menú interactivo.
// CÓMO: Imprime opciones y espera entrada del usuario.
// POR QUÉ: Proporciona una interfaz simple para interactuar con el programa.
void mostrarMenu() {
    printf("\n--- Plataforma de Edición de Imágenes ---\n");
    printf("1. Cargar imagen PNG\n");
    printf("2. Mostrar matriz de píxeles\n");
    printf("3. Guardar como PNG\n");
    printf("4. Ajustar brillo (+/- valor) concurrentemente\n");
    printf("5. Salir\n");
    printf("Opción: ");
}

// QUÉ: Función principal que controla el flujo del programa.
// CÓMO: Maneja entrada CLI, ejecuta el menú en bucle y llama funciones según opción.
// POR QUÉ: Centraliza la lógica y asegura limpieza al salir.
int main(int argc, char* argv[]) {
    ImagenInfo imagen = {0, 0, 0, NULL}; // Inicializar estructura
    char ruta[256] = {0}; // Buffer para ruta de archivo

    // QUÉ: Cargar imagen desde CLI si se pasa.
    // CÓMO: Copia argv[1] y llama cargarImagen.
    // POR QUÉ: Permite ejecución directa con ./img imagen.png.
    if (argc > 1) {
        strncpy(ruta, argv[1], sizeof(ruta) - 1);
        if (!cargarImagen(ruta, &imagen)) {
            return EXIT_FAILURE;
        }
    }

    int opcion;
    while (1) {
        mostrarMenu();
        // QUÉ: Leer opción del usuario.
        // CÓMO: Usa scanf y limpia el buffer para evitar bucles infinitos.
        // POR QUÉ: Manejo robusto de entrada evita errores comunes.
        if (scanf("%d", &opcion) != 1) {
            while (getchar() != '\n');
            printf("Entrada inválida.\n");
            continue;
        }
        while (getchar() != '\n'); // Limpiar buffer

        switch (opcion) {
            case 1: { // Cargar imagen
                printf("Ingresa la ruta del archivo PNG: ");
                if (fgets(ruta, sizeof(ruta), stdin) == NULL) {
                    printf("Error al leer ruta.\n");
                    continue;
                }
                ruta[strcspn(ruta, "\n")] = 0; // Eliminar salto de línea
                liberarImagen(&imagen); // Liberar imagen previa
                if (!cargarImagen(ruta, &imagen)) {
                    continue;
                }
                break;
            }
            case 2: // Mostrar matriz
                mostrarMatriz(&imagen);
                break;
            case 3: { // Guardar PNG
                char salida[256];
                printf("Nombre del archivo PNG de salida: ");
                if (fgets(salida, sizeof(salida), stdin) == NULL) {
                    printf("Error al leer ruta.\n");
                    continue;
                }
                salida[strcspn(salida, "\n")] = 0;
                guardarPNG(&imagen, salida);
                break;
            }
            case 4: { // Ajustar brillo
                int delta;
                printf("Valor de ajuste de brillo (+ para más claro, - para más oscuro): ");
                if (scanf("%d", &delta) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                while (getchar() != '\n');
                ajustarBrilloConcurrente(&imagen, delta);
                break;
            }
            case 5: // Salir
                liberarImagen(&imagen);
                printf("¡Adiós!\n");
                return EXIT_SUCCESS;
            default:
                printf("Opción inválida.\n");
        }
    }
    liberarImagen(&imagen);
    return EXIT_SUCCESS;
}