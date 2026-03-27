# LegacyCraft

Personal development fork of **Minecraft Legacy Console Edition** (October 2014 backup, ~TU19).  
This repo tracks UI improvements and custom features added on top of the original PC/Windows 64-bit build.

> **Private repo** — Source code is proprietary to Mojang/Microsoft. This is for personal version control only.

---

## Custom UI System

All custom controls live directly in `Minecraft.Client/` and are registered in `Minecraft.Client.vcxproj`.

---

### `CustomGenericButton` — Button con textura PNG

Botón reutilizable que renderiza una textura normal + hover, con texto centrado.

**Archivos:** `CustomGenericButton.h` / `CustomGenericButton.cpp`

```cpp
CustomGenericButton myButton;

// Botón genérico (textura + tamaño explícitos)
myButton.Setup(L"/Graphics/MyBtn_Norm.png", L"/Graphics/MyBtn_Over.png",
               x, y, width, height);

// Botón de menú principal (usa MainMenuButton_Norm/Over.png, altura automática)
myButton.SetupMenuButton(x, y);

// En tick():
if(myButton.Update(minecraft)) { /* click */ }

// En render():
myButton.Render(minecraft, font, L"Texto", viewport);
```

| Estado | Textura | Texto |
|---|---|---|
| Normal | `*_Norm.png` | Blanco |
| Hover | `*_Over.png` | Amarillo |

**Texturas usadas por `SetupMenuButton`:**
- `Graphics/MainMenuButton_Norm.png` — fondo gris oscuro
- `Graphics/MainMenuButton_Over.png` — fondo azul/violeta

---

### `CustomSlider` — Slider con rango configurable

Control de slider horizontal con fondo de pista, handle arrastrable, overlay de hover y label centrado.

**Archivos:** `CustomSlider.h` / `CustomSlider.cpp`

```cpp
CustomSlider mySlider;

// Tamaño por defecto (600×32), rango 0-100
mySlider.Setup(x, y);

// Rango personalizado
mySlider.Setup(x, y, 200);           // 0-200
mySlider.Setup(x, y, 5);            // 0-5

// Tamaño y rango explícitos
mySlider.Setup(x, y, 480.0f, 40.0f, 100);

// En tick():
bool changed = mySlider.Update(minecraft);

// En render() — muestra "[string]: [valor]%" o "[string]: [valor]/[max]"
mySlider.Render(minecraft, font, IDS_MY_STRING, viewport);

// Leer el valor actual:
int value = mySlider.GetValue();  // 0 … limitMax
```

| Estado | Fondo | Borde | Texto |
|---|---|---|---|
| Normal | `Slider_Track.png` | Ninguno | Blanco |
| Hover | `LeaderboardButton_Over.png` encima | Amarillo 2px | Amarillo |

**Texturas:**
- `Graphics/Slider_Track.png` — pista de fondo (oscuro)
- `Graphics/Slider_Button.png` — handle (el nodo que se arrastra)
- `Graphics/LeaderboardButton_Over.png` — overlay hover (azul)

---

## Cambios en `UIScene_MainMenu`

- **6 botones de menú custom** con `CustomGenericButton::SetupMenuButton()`, reemplazando los botones originales de Iggy/Flash
- **Guard de escena activa** — los botones solo se actualizan/renderizan cuando el menú principal tiene el foco, evitando interacción fantasma desde submenús
- **Slider de prueba** en `(0, 0)` para testing en desarrollo (fácil de quitar)

---

## Estructura de archivos relevantes

```
Minecraft.Client/
├── CustomGenericButton.h / .cpp    ← Botón genérico con PNG
├── CustomSlider.h / .cpp           ← Slider con rango y hover
└── Common/
    └── UI/
        └── UIScene_MainMenu.cpp    ← Integración de los controles custom
```

---

## Build

- **IDE:** Visual Studio 2012 (v110 toolset)
- **Plataforma:** Windows x64
- **Proyecto:** `Minecraft.Client/Minecraft.Client.vcxproj`

```
Configuración recomendada: Release | x64
```
