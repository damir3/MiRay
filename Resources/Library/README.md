# MiRay Asset Library

This directory contains the standard resource assets and presets bundled with the **MiRay** rendering engine.

These assets appear in the MiRay user interface in the left library panel and are accessible via the `miray://Library/` URI scheme. You can also open this folder directly from within MiRay via the menu: **Help -> Open library folder...**.

---

## Directory Structure

| Directory | Description | Supported Extensions |
| :--- | :--- | :--- |
| [`Environments/`](Environments/) | High Dynamic Range panoramic lighting maps (HDRIs) | `.hdr`, `.exr` |
| [`Materials/`](Materials/) | Preconfigured material presets (Glass, Plastic, Metal, Stone, etc.) | `.mirayMaterial` |
| [`Shapes/`](Shapes/) | 3D shape primitives, test stages, and studio setups | `.mirayScene` |
| [`Textures/`](Textures/) | Surface textures, normal maps, brushed metal patterns, and procedural test patterns | `.jpg`, `.png`, `.exr` |

---

## Third-Party Assets & Attribution

### Poly Haven Assets (HDRIs & 3D Models)

The following assets are provided by **[Poly Haven](https://polyhaven.com)** (formerly *HDRI Haven*):

- **Environment Lighting Maps**: All HDR maps in [`Environments/`](Environments/) (Indoor, Outdoor, Studio).
- **3D Models & Textures**: `Apple` (*Apple 01*), `Round Table` (*Round Wooden Table 01*), and `Table` (*Wooden Table 02*) with their PBR textures in [`Shapes/Objects/`](Shapes/Objects/).

**Licensing & Links**:
- **Source**: [https://polyhaven.com](https://polyhaven.com)
- **License**: [Creative Commons CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/) (Public Domain Dedication)
- **License Details**: See [`LICENSE.md`](LICENSE.md) or visit [https://polyhaven.com/license](https://polyhaven.com/license)
- **Support**: If you find these assets valuable, consider supporting Poly Haven on [Patreon](https://www.patreon.com/polyhaven).

Under the **CC0** license, these assets may be used for any purpose, including commercial projects, without mandatory attribution or permission requirements.
