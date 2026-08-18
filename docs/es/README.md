# Weqi

**Weqi** es una aplicación de ajedrez de escritorio moderna, limpia y de código abierto. Todas las reglas del ajedrez están implementadas en un motor C++ local, con soporte para humano contra humano, humano contra IA, IA contra IA y reproducción de partidas.

> **Lea este documento en su idioma:**
> [English](../../README.md) · [简体中文](../zh-CN/README.md) · [繁體中文](../zh-TW/README.md) · [日本語](../ja/README.md) · [Українська](../uk/README.md) · [한국어](../ko/README.md)

---

## Características

- **Cuatro modos de juego**
  - **Humano contra humano** — dos jugadores en el mismo dispositivo.
  - **Humano contra IA** — juega contra un proveedor de IA de tu elección.
  - **IA contra IA** — dos IAs juegan automáticamente (no cuenta para tus estadísticas).
  - **Reproducción** — revisa tus partidas históricas.
- **Reglas completas del ajedrez** implementadas localmente en C++ (validación de movimientos legales, jaque, jaque mate, ahogado, enroque, captura al paso, promoción, etc.).
- **Perfil de jugador y rating** — sigue tu rating, tu mejor rating y edita tu nombre de jugador.
- **Estadísticas** — partidas jugadas, porcentaje de victorias, victorias, tablas, derrotas, mejor/racha actual de victorias, mejor rating.
- **Historial de partidas** — explora partidas pasadas con fecha, modo, oponente, resultado y cambio de rating.
- **Continuar partida sin terminar** — reanuda una partida en curso en cualquier momento.
- **Proveedores de IA** — añade, edita y elimina proveedores de IA (nombre, tipo, URL base, clave API, modelo). Las claves API se guardan de forma privada en el directorio de datos del usuario del sistema, nunca en el proyecto.
- **Chat de IA** — la IA comenta brevemente su movimiento; el mensaje sigue el idioma de tu interfaz.
- **Internacionalización (i18n)** — 7 idiomas de interfaz: chino simplificado, chino tradicional, inglés, japonés, español, ucraniano y coreano. Cambia de idioma en tiempo de ejecución sin reiniciar.

## Pila tecnológica

- **C++17**
- **Qt 6** (Widgets)
- **CMake**

## Estructura de directorios

```
weqi/
├── CMakeLists.txt
├── README.md
├── ai_adapter/          # Adaptador de IA en Python (detección de idioma, reintento/ajuste)
│   ├── main.py
│   ├── parser.py
│   └── providers/
│       └── openai_compatible.py
├── resources/           # Archivos de recursos Qt
├── scripts/             # Scripts auxiliares
├── src/
│   ├── main.cpp         # Punto de entrada del programa
│   ├── MainWindow.h/.cpp
│   ├── GameController.h/.cpp
│   ├── ChessBoard.h/.cpp
│   ├── ChessPiece.h/.cpp
│   ├── ai/              # Gestor de IA
│   ├── data/            # Gestores de configuración, idioma, perfil, estadísticas e historial
│   └── pages/           # Inicio, nueva partida, partida, humano-IA, IA-IA, historial, configuración, acerca de
├── tests/               # Pruebas C++
├── tools/               # Herramientas de desarrollo
└── translations/        # Archivos de traducción Qt .ts (7 idiomas)
```

## Compilación

### Dependencias

- CMake ≥ 3.16
- Qt 6 (≥ 6.2, incluido el módulo Widgets)
- Un compilador compatible con C++17 (GCC / Clang)
- Python 3 (para el adaptador de IA)

### Pasos de compilación

```bash
# 1. Configurar
cmake -S . -B build

# 2. Compilar
cmake --build build -j

# 3. Ejecutar
./build/Weqi
```

O como una sola línea:

```bash
cmake -S . -B build && cmake --build build -j && ./build/Weqi
```

### Traducciones

Los archivos de traducción están en `translations/` y se compilan en el binario mediante `qt_add_translations` de Qt. Después de cambiar las cadenas `tr()`, actualiza las traducciones:

```bash
# Actualizar archivos .ts (extraer nuevas cadenas)
lupdate src -ts translations/weqi_<lang>.ts

# Publicar archivos .qm
lrelease translations/weqi_<lang>.ts
```

## Uso

- **Seleccionar una pieza**: haz clic izquierdo en cualquier pieza para resaltarla.
- **Mover una pieza**: después de seleccionar, haz clic en una casilla de destino. El motor local valida los movimientos legales.
- **Deshacer**: usa el botón «Deshacer» para retirar un movimiento.
- **Nueva partida**: inicia una nueva partida desde la página de juego o la pantalla de inicio.
- **IA contra IA**: elige dos proveedores de IA y usa Iniciar / Pausar / Reanudar / Detener para controlar la partida automática.
- **Cambiar idioma**: abre Configuración → Idioma y elige uno de los 7 idiomas compatibles (o «Seguir sistema»).

## Almacenamiento de datos

- **Configuración y perfil**: se guardan en el directorio de datos del usuario del sistema (por ejemplo, `~/.local/share/Weqi/` en Linux).
- **Configuración de proveedores de IA**: se guarda por separado en el directorio de datos del usuario, nunca en el proyecto.
- **Historial de partidas y partidas guardadas**: se guardan en el directorio de datos del usuario.

## Licencia

Código abierto. Consulta el repositorio para más detalles.
