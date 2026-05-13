# Laboratorio #4 - Métricas de Rendimiento
# Wilson Chavarría Miranda C22114
# Fabricio Padilla Madrigal C35837

## Introducción

En este laboratorio se tomó como base el aeropuerto concurrente desarrollado anteriormente y se aumentó drásticamente la cantidad de vuelos y el flujo de equipaje para analizar si el sistema realmente escala al agregar más pistas y más personal de tierra.

La idea principal fue medir:

- Speedup
- Eficiencia
- Ley de Amdahl
- Ley de Gustafson-Barsis
- Overhead por sincronización
- Isoeficiencia

Para medir los tiempos se utilizó `clock_gettime(CLOCK_MONOTONIC)` para trabajar con wall clock time, tal como lo recomienda Pacheco.

Se decidió bajar drásticamente el tiempo de los procesos ya que habían ejecuciones que podían durar horas.
---

# Instrumentación

Se agregaron mediciones de tiempo para:

- Tiempo de inicialización
- Tiempo paralelo
- Tiempo de destrucción

El tiempo paralelo se midió desde los `pthread_create` hasta los `pthread_join`.

El tiempo serial puro se calculó con:

```c
Tiempo serial puro = tiempo_init + tiempo_destroy
```

Además, para obtener el `Tserial` utilizado en todas las fórmulas, se ejecutó el sistema con:

- 1 pista
- 1 personal de tierra
- barreras desactivadas
- mantenimientos seriales

El comando utilizado fue:

```bash
./aeropuerto 1000 1 5 1 1
```

Resultado:

```text
Tserial = 24.074466 s
```

---

# 1. Speedup y eficiencia

## Pregunta

Fijar el número de aviones en 1000 y variar:

```text
p = 1, 2, 4, 8, 16
```

Luego calcular:

```text
Speedup = Tserial / Tparalelo
Eficiencia = Speedup / p
```

---

## Cálculo usado

Ejemplo con `p = 4`:

```text
Speedup = 24.074466 / 4.161864
Speedup = 5.7845
```

```text
Eficiencia = 5.7845 / 4
Eficiencia = 1.4461
```

---

## Resultados

| p | Tparalelo (s) | Cálculo Speedup | Speedup | Cálculo Eficiencia | Eficiencia |
|---|---:|---|---:|---|---:|
| 1 | 16.564601 | 24.074466 / 16.564601 | 1.4534 | 1.4534 / 1 | 1.4534 |
| 2 | 8.273460 | 24.074466 / 8.273460 | 2.9105 | 2.9105 / 2 | 1.4553 |
| 4 | 4.161864 | 24.074466 / 4.161864 | 5.7845 | 5.7845 / 4 | 1.4461 |
| 8 | 2.574433 | 24.074466 / 2.574433 | 9.3514 | 9.3514 / 8 | 1.1689 |
| 16 | 2.512243 | 24.074466 / 2.512243 | 9.5833 | 9.5833 / 16 | 0.5990 |

También se realizaron pruebas extra:

| p | Tparalelo (s) | Speedup | Eficiencia |
|---|---:|---:|---:|
| 32 | 2.595726 | 9.2746 | 0.2898 |
| 64 | 2.661740 | 9.0446 | 0.1413 |

---

## Respuesta

Se observó que el sistema escala bastante bien hasta aproximadamente 8 pistas.

A partir de 16 pistas el rendimiento prácticamente deja de mejorar.

Incluso con 32 y 64 pistas el tiempo paralelo empeora ligeramente, lo que demuestra que agregar más hardware no siempre significa mejor rendimiento.

Esto ocurre debido a overhead relacionado con:

- sincronización
- mutex
- semáforos
- creación de hilos
- contención en la banda de equipaje

La eficiencia también cae bastante conforme aumenta `p`, especialmente después de 16 pistas.

---

# 2. Ley de Amdahl

## Pregunta

Usando el tiempo de inicialización y destrucción, calcular la fracción serial y el speedup máximo teórico si se compraran pistas infinitas.

---

## Datos usados

Se tomó el caso de `p = 16`:

```text
Tiempo init = 0.000029 s
Tiempo destroy = 0.000001 s
Tiempo paralelo = 2.512243 s
```

Entonces:

```text
Tiempo serial puro = 0.000029 + 0.000001
Tiempo serial puro = 0.000030 s
```

---

## Cálculo de la fracción serial

```text
s = tiempo serial puro / tiempo paralelo
```

```text
s = 0.000030 / 2.512243
s = 0.00001194
```

---

## Cálculo de Amdahl

```text
Smax = 1 / s
```

```text
Smax = 1 / 0.00001194
Smax ≈ 83752.75
```

---

## Respuesta

El speedup máximo teórico según la ley de Amdahl sería aproximadamente:

```text
83752.75
```

Sin embargo, experimentalmente el mayor speedup observado fue aproximadamente:

```text
9.5833
```

Esto ocurre porque Amdahl no toma en cuenta overheads reales como:

- mutex
- semáforos
- contención
- cambios de contexto
- administración de hilos
- sincronización

Por eso el resultado teórico termina siendo demasiado optimista comparado con lo que ocurre realmente.

---

# 3. Gustafson-Barsis

## Pregunta

Escalar el problema y el hardware:

```text
p = 1  -> M = 100 vuelos
p = 4  -> M = 400 vuelos
p = 8  -> M = 800 vuelos
```

Aplicar la ley de Gustafson-Barsis y demostrar si el sistema rompe el techo del punto anterior.

---

## Fórmula usada

```text
S_G = p - s(p - 1)
```

donde:

```text
s = tiempo serial puro / tiempo paralelo
```

---

## Cálculos

### Caso p = 1

Datos:

```text
Tiempo serial puro = 0.000019
Tiempo paralelo = 1.674047
```

```text
s = 0.000019 / 1.674047
s = 0.00001135
```

```text
S_G = 1 - 0.00001135(1 - 1)
S_G = 1
```

---

### Caso p = 4

Datos:

```text
Tiempo serial puro = 0.000018
Tiempo paralelo = 1.643766
```

```text
s = 0.000018 / 1.643766
s = 0.00001095
```

```text
S_G = 4 - 0.00001095(4 - 1)
S_G = 4 - 0.00003285
S_G = 3.99997
```

---

### Caso p = 8

Datos:

```text
Tiempo serial puro = 0.000014
Tiempo paralelo = 2.020473
```

```text
s = 0.000014 / 2.020473
s = 0.00000693
```

```text
S_G = 8 - 0.00000693(8 - 1)
S_G = 8 - 0.00004851
S_G = 7.99995
```

---

## Resultados

| p | Vuelos | Tparalelo (s) | Tiempo serial puro | s | Speedup Gustafson |
|---|---:|---:|---:|---:|---:|
| 1 | 100 | 1.674047 | 0.000019 | 0.00001135 | 1.0000 |
| 4 | 400 | 1.643766 | 0.000018 | 0.00001095 | 3.99997 |
| 8 | 800 | 2.020473 | 0.000014 | 0.00000693 | 7.99995 |

---

## Respuesta

Sí, con Gustafson-Barsis el sistema logra mostrar una mejor escalabilidad.

Cuando se aumenta el número de pistas y también aumenta el número de vuelos, el trabajo útil crece muchísimo más que la parte serial.

Por eso:

- con `p = 4` el speedup fue prácticamente `4`
- con `p = 8` el speedup fue prácticamente `8`

Esto demuestra que el sistema sí puede aprovechar más hardware si también aumenta el tamaño del problema.

---

# 4. Colapso de equipaje y fracción serial empírica

## Pregunta

El aeropuerto colapsa cuando se intenta usar muchas pistas y consumidores con un millón de maletas.

Calcular la fracción serial empírica `e` para:

```text
p = 2, 4, 6, 8, 16
```

---

## Datos usados

Para este escenario se cambió temporalmente:

```c
#define MALETAS 1000
```

Entonces:

```text
1000 vuelos * 1000 maletas = 1,000,000 maletas
```

La banda se mantuvo con tamaño:

```text
TAMANO_BANDA = 5
```

---

## Fórmulas usadas

Primero:

```text
S = Tserial / Tparalelo
```

Luego:

```text
e = ((1/S) - (1/p)) / (1 - (1/p))
```

Se usó:

```text
Tserial = 24.074466 s
```

---

## Ejemplo de cálculo con p = 4

```text
S = 24.074466 / 534.719521
S = 0.04502
```

```text
e = ((1 / 0.04502) - (1 / 4)) / (1 - (1 / 4))
```

```text
e = (22.2123 - 0.25) / 0.75
e = 29.283
```

Aproximando:

```text
e ≈ 28.96
```

---

## Resultados

| p | Tparalelo (s) | Cálculo Speedup | Speedup | e |
|---|---:|---|---:|---:|
| 2 | 1126.536442 | 24.074466 / 1126.536442 | 0.02137 | 92.59 |
| 4 | 534.719521 | 24.074466 / 534.719521 | 0.04502 | 28.96 |
| 6 | 352.097045 | 24.074466 / 352.097045 | 0.06838 | 16.35 |
| 8 | 263.144273 | 24.074466 / 263.144273 | 0.09149 | 11.35 |
| 16 | 132.105680 | 24.074466 / 132.105680 | 0.18224 | 5.79 |

---

## Respuesta

En este caso el sistema sí presentó un cuello de botella bastante fuerte.

Aunque aumentar las pistas y el personal sí ayudó a reducir el tiempo, la mejora dejó de ser proporcional.

La principal causa fue la banda de equipaje.

En el código, tanto productores como consumidores compiten constantemente por:

```c
pthread_mutex_lock(&mutex)
```

Además, los semáforos hacen que los hilos se bloqueen:

```c
sem_wait(&vacia)
sem_wait(&llena)
```

Cuando la banda está llena, los productores esperan.

Cuando la banda está vacía, los consumidores quedan bloqueados esperando nuevas maletas.

El tamaño pequeño de la banda (`5`) empeoró muchísimo la contención y terminó provocando un cuello de botella importante.

---

# 5. Isoeficiencia

## Pregunta

Para mantener una eficiencia del 80%, al pasar de 4 a 8 pistas, ¿a qué ritmo debe aumentar el tamaño del problema?

Se indica que el overhead de sincronización en los semáforos aumenta linealmente con los hilos.

---

## Fórmula base

La eficiencia puede verse como:

```text
E = W / (W + To)
```

donde:

```text
W  = trabajo útil
To = overhead
```

Si se quiere:

```text
E = 0.80
```

Entonces:

```text
0.80 = W / (W + To)
```

Resolviendo:

```text
0.80(W + To) = W
```

```text
0.80W + 0.80To = W
```

```text
0.80To = 0.20W
```

```text
W = 4To
```

---

## Interpretación

Si el overhead crece linealmente con los hilos:

```text
To = O(p)
```

entonces el trabajo también debe crecer linealmente:

```text
W = O(p)
```

---

## Respuesta

Al pasar de 4 pistas a 8 pistas, el número de pistas se duplica.

Por lo tanto, para mantener una eficiencia cercana al 80%, el número de vuelos también debería duplicarse aproximadamente.

Ejemplo:

| Pistas | Vuelos |
|---|---:|
| 4 | 1000 |
| 8 | 2000 |

Si se agregan pistas pero no se aumenta el número de vuelos, las nuevas pistas se vuelven desperdicio de recursos porque el overhead empieza a pesar más que el trabajo útil.

---

# Conclusión general

Con las pruebas realizadas se puede concluir que el aeropuerto sí escala, pero solamente hasta cierto punto.

En el primer escenario se observó que el sistema mejora bastante hasta aproximadamente 8 pistas, pero después de 16 pistas el beneficio empieza a caer.

La ley de Amdahl mostró un speedup máximo teórico extremadamente alto, pero experimentalmente eso no ocurre debido al overhead real del sistema.

Gustafson-Barsis demostró que el sistema sí puede aprovechar mejor el hardware cuando también aumenta el tamaño del problema.

El mayor cuello de botella apareció en la banda de equipaje, especialmente cuando se trabajó con un millón de maletas y una banda muy pequeña.

Finalmente, la isoeficiencia mostró que no basta con comprar más pistas o contratar más personal. También es necesario aumentar proporcionalmente la carga de trabajo para que el nuevo hardware realmente valga la pena.