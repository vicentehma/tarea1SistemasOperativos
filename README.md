# El Planificador Dieciochero

**Tarea 1 — Sistemas Operativos**
Procesos, Tuberías (Pipes) y Señales

Simulador y planificador de actividades modeladas como un Grafo Acíclico
Dirigido (DAG). El programa lee un plan de actividades con sus dependencias y
duraciones, y las ejecuta como procesos respetando un límite de concurrencia,
comunicando a cada actividad con sus dependientes mediante pipes. Incluye
manejo de la señal SIGINT y tolerancia a fallos por rama.

Toda la concurrencia está implementada con **procesos** (`fork`), sin usar
hilos ni mecanismos de sincronización de hilos, tal como exige la tarea.
## Integrantes
- Vicente Hernandez
## Requisitos del sistema

- Sistema operativo Linux (probado en WSL - Ubuntu).
- Compilador g++ con soporte para C++17.
## Estructura del proyecto

    planificador/
    |-- tarea.cpp      Codigo fuente completo del planificador
    |-- Makefile       Reglas de compilacion
    |-- plan.txt       Plan de ejemplo (el del enunciado)
    |-- README.md      Este archivo
    |-- .gitignore     Ignora el binario compilado

---

## Compilación

Con el Makefile incluido (recomendado):

    make

Esto ejecuta internamente la compilación estricta:

    g++ -Wall -Wextra -std=c++17 -o planificador tarea.cpp -lpthread

Para borrar el binario y volver a compilar desde cero:

    make clean
    make

El programa compila sin ningún warning con los flags `-Wall -Wextra`.

---

## Modo de uso

    ./planificador plan.txt K

Argumentos:

- `plan.txt` : ruta al archivo de texto con la descripcion del DAG.
- `K`        : limite de concurrencia. Numero maximo de procesos (actividades)
               que pueden estar ejecutandose al mismo tiempo.

Ejemplo:

    ./planificador plan.txt 3

Si se invoca con un numero incorrecto de argumentos, el programa muestra el
modo de uso y termina:

    Uso: ./planificador plan.txt K

---

## Formato del archivo plan.txt

Cada linea del archivo describe una actividad del grafo, con cuatro campos
separados por el caracter `:` --

    ID : Nombre : tiempo_ms : [Dependencia1, Dependencia2, ...]

Detalle de los campos:

- **ID**: identificador unico de la actividad (alfanumerico).
- **Nombre**: etiqueta descriptiva de la actividad.
- **tiempo_ms**: duracion estimada en milisegundos. Si se deja vacio, el
  programa le asigna un valor aleatorio entre 100 y 5000 ms.
- **Dependencias**: lista de IDs separados por coma. La actividad solo puede
  comenzar cuando todas sus dependencias han finalizado. Puede ir vacio.

Ejemplo (plan.txt del enunciado):

    1 : prender_carbon : 500 :
    2 : comprar_carne : 1200 :
    3 : comprar_pan : 300 :
    4 : asar_longaniza : 800 : 1, 2
    5 : armar_choripan : 250 : 3, 4
    6 : servir_mesa : 100 : 5

En este ejemplo, asar_longaniza (4) no puede comenzar hasta que el carbon
este prendido (1) y la carne comprada (2).

---

## Ejemplos de ejecución

### Ejecucion normal

    ./planificador plan.txt 2

Cada actividad imprime cuando inicia y cuando termina, y cada actividad
dependiente muestra los insumos que recibio por pipe:

    [INICIO] prender_carbon
    [FIN]    prender_carbon
    ...
    [PIPE] asar_longaniza recibio: prender_carbon listo
    comprar_carne listo
    [INICIO] asar_longaniza
    ...
    Todas las actividades completadas.

### Probar distintos valores de K

    ./planificador plan.txt 1     # ejecucion estrictamente secuencial
    ./planificador plan.txt 5     # hasta 5 actividades en paralelo

### Probar la tolerancia a fallos

Para forzar que una actividad falle y observar como se aborta unicamente su
rama, se usa la variable de entorno FALLA_ID con el ID de la actividad que
debe fallar:

    FALLA_ID=4 ./planificador plan.txt 2

Salida (resumida):

    [INICIO] asar_longaniza
    [FALLO]  asar_longaniza fallo internamente
    [ABORTO] rama de asar_longaniza cancelada
       -> abortada: armar_choripan
       -> abortada: servir_mesa

Las actividades que no dependen de la fallida continuan normalmente.

### Probar la señal SIGINT (Ctrl+C)

Durante una ejecucion con actividades largas, al presionar Ctrl+C:

    ^C
    [SEREMI] Llego la autoridad! Abortando todas las actividades...
    [SEREMI] Celebracion clausurada.

El planificador mata a todos los procesos hijos vivos y termina.

---

## Funciones implementadas

A continuacion se detalla como cumple cada requisito del enunciado.

### 1. Lectura del DAG (seccion 2)

El programa lee el plan.txt linea por linea y separa los cuatro campos usando
el caracter `:` como delimitador. Cada campo se limpia de espacios sobrantes
(el formato del enunciado usa espacios alrededor de los `:`). Las dependencias
se separan ademas por comas. A las actividades sin duracion se les asigna un
tiempo aleatorio entre 100 y 5000 ms con un generador Mersenne Twister.

### 2. Control de concurrencia K (seccion 3.1)

El proceso padre mantiene un contador de procesos vivos. Mientras haya
actividades listas (con todas sus dependencias cumplidas) y el numero de
procesos vivos sea menor que K, lanza nuevas actividades con fork. Cuando se
alcanza el limite K, el padre se bloquea en waitpid hasta que algun hijo
termine, liberando un cupo. Asi nunca hay mas de K procesos simultaneos.

### 3. Paso de mensajes por pipes (seccion 3.2)

Cada actividad que tiene dependencias posee un pipe que actua como su "buzon
de insumos". Cuando una actividad termina su simulacion, escribe un mensaje de
texto acotado (por ejemplo "prender_carbon listo") en el buzon de cada una de
sus actividades dependientes. Cuando una actividad dependiente reune todos sus
insumos, el padre lee esos mensajes desde su buzon antes de ejecutarla.

### 4. Carga de trabajo de 10.000 actividades (seccion 3.3)

El programa fue probado con planes de 10.000 actividades. Para soportar esta
carga sin agotar el limite de descriptores de archivo del sistema, los pipes
se crean y se cierran de forma dinamica (ver decisiones de diseño).

### 5. Inspeccion de la Seremi - SIGINT (seccion 4.2)

Se instala un manejador para la señal SIGINT (Ctrl+C). Al recibirla, el padre
envia SIGKILL a todos los procesos hijos vivos, los recolecta con wait y
termina. Simula la llegada de la autoridad que clausura la celebracion.

### 6. Aislamiento de errores - tolerancia a fallos (seccion 4.1)

Si una actividad termina con codigo de error, el padre lo detecta con
WIFEXITED y WEXITSTATUS. En vez de cerrarse, marca en cascada como "abortadas"
todas las actividades que dependian (directa o indirectamente) de la fallida,
y continua ejecutando el resto del plan.

---

## Arquitectura y funcionamiento interno

El nucleo del programa es un bucle de planificacion ejecutado por el proceso
padre. El flujo general es:

1. Se parsea el plan.txt y se construye el grafo en memoria. Para cada
   actividad se calcula cuantas dependencias le faltan (deps_pendientes) y la
   lista de actividades que dependen de ella (dependientes).

2. Las actividades sin dependencias entran a una lista de "listas para
   ejecutar".

3. El bucle principal repite, mientras queden actividades por procesar:
   - Si llego SIGINT, mata a todos los hijos y termina.
   - Lanza actividades listas con fork, sin superar K procesos vivos. Antes de
     cada fork, crea los buzones (pipes) de los dependientes de la actividad
     que se va a lanzar, para que el hijo herede el extremo de escritura.
   - El hijo imprime INICIO, simula su duracion con usleep, escribe su mensaje
     de insumo en el buzon de cada dependiente y termina con _exit.
   - El padre espera con waitpid a que termine un hijo. Segun el resultado:
     - Si el hijo fallo: aborta en cascada su rama de dependientes.
     - Si termino bien: decrementa el contador de dependencias de sus
       dependientes; los que llegan a cero leen sus insumos del buzon y pasan
       a la lista de listos.

---

## Estructuras de datos

- **struct Actividad**: representa un nodo del grafo. Guarda id, nombre,
  tiempo, lista de dependencias, contador de dependencias pendientes, lista de
  dependientes, banderas de estado (lanzada, abortada) y los descriptores de
  su pipe (buzon).

- **vector<Actividad>**: contiene todas las actividades del plan.

- **unordered_map<string,int> indice**: mapea el ID de cada actividad a su
  posicion en el vector, para resolver dependencias en tiempo constante
  (necesario para escalar a 10.000 actividades).

- **unordered_map<pid_t,int> pid_a_pos**: asocia el PID de cada proceso hijo
  con la posicion de su actividad, para saber que actividad termino cuando
  waitpid retorna.

- **vector<int> listas**: cola de actividades listas para ejecutar.

---

## Decisiones de diseño

- **Procesos, no hilos**: toda la concurrencia usa fork, waitpid y pipe. No se
  usan hilos ni primitivas de sincronizacion de hilos. El flag -lpthread se
  incluye unicamente porque la pauta lo exige en la linea de compilacion; el
  programa no crea ningun hilo (se puede verificar: no hay include de <thread>
  ni llamadas a pthread_create).

- **El padre como orquestador central**: el proceso padre mantiene todo el
  estado del grafo y toma las decisiones; los hijos solo simulan su actividad
  y mueren. Esto mantiene la logica de coordinacion en un solo lugar y evita
  condiciones de carrera sobre el estado del grafo.

- **Control de K con waitpid bloqueante**: en lugar de espera activa o
  semaforos, cuando se alcanzan K procesos el padre simplemente se bloquea en
  waitpid. Es simple, eficiente y garantiza el limite de forma estricta.

- **Pipes como buzon por actividad con apertura y cierre dinamico**: en vez de
  crear los pipes de todas las actividades al inicio (lo que con 10.000
  actividades agotaria el limite de descriptores del SO, tipicamente 1024), el
  buzon de una actividad se crea solo cuando una de sus dependencias va a ser
  lanzada, y se cierra apenas la actividad consume sus insumos. Asi, en todo
  momento solo hay unos pocos buzones abiertos.

- **Lectura exacta de insumos**: como el padre sabe cuantas dependencias tiene
  cada actividad, lee exactamente esa cantidad de mensajes del buzon (contando
  saltos de linea), sin depender de EOF. Esto evita bloqueos por descriptores
  de escritura heredados que pudieran seguir abiertos.

- **Señal manejada con una bandera**: el manejador de SIGINT solo levanta una
  bandera de tipo volatile sig_atomic_t, que es lo unico seguro de hacer dentro
  de un handler. El trabajo real (matar hijos, imprimir) se realiza en el bucle
  principal. Se usa sigaction sin la opcion SA_RESTART para que waitpid se
  interrumpa de inmediato al llegar la señal, y asi la bandera se revise sin
  esperar a que termine un hijo.

- **Manejo del buffer de salida en fork**: antes de cada fork el padre vacia su
  buffer de salida (flush), y cada hijo hace flush antes de _exit. Esto evita
  que los mensajes se pierdan o se dupliquen cuando la salida se redirige a un
  archivo (donde la salida pasa a estar completamente bufferizada).

- **Fallo simulado por variable de entorno**: para no alterar el formato
  oficial del plan.txt (cuatro campos) ni la invocacion oficial
  ./planificador plan.txt K, el fallo de una actividad se simula de forma
  interna y se controla con la variable de entorno FALLA_ID. Por defecto, sin
  esa variable, el programa ejecuta todas las actividades normalmente, lo que
  permite correr las pruebas de estrés sin interferencia.

---

## Manejo de casos especiales

- Lineas vacias en el plan.txt: se ignoran.
- Espacios alrededor de los `:` y las comas: se limpian al parsear.
- Actividades sin duracion: reciben un tiempo aleatorio entre 100 y 5000 ms.
- Actividades sin dependencias: entran listas desde el inicio.
- Archivo inexistente o argumentos invalidos: el programa informa y termina
  con codigo de error.

---

## Limitaciones conocidas

- El numero de buzones (pipes) abiertos simultaneamente depende de la forma del
  grafo. En grafos extremadamente anchos (muchisimas actividades esperando a la
  vez) podria crecer; el cierre temprano de buzones mantiene este numero
  acotado para cargas normales y en cadena.
- Los mensajes de insumo tienen un tamaño acotado (buffer de lectura de 256
  bytes por lectura), suficiente para los nombres de actividad usados.

---

## Pruebas realizadas

- Ejecucion con el plan del enunciado y distintos valores de K (1, 2, 3, 5),
  verificando que se respeta el orden de dependencias y el limite de
  concurrencia.
- Prueba de estrés con 10.000 actividades, verificando que el programa termina
  correctamente sin agotar descriptores.
- Prueba de tolerancia a fallos con FALLA_ID, verificando que solo se aborta la
  rama dependiente de la actividad fallida y que las ramas paralelas continuan.
- Prueba de SIGINT (Ctrl+C), verificando que todos los procesos hijos son
  eliminados y no quedan procesos huerfanos.
- Compilacion con -Wall -Wextra sin warnings.
