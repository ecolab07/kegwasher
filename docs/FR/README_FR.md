# Laveuse de fûts automatique

> 🇬🇧 [English version](../../README.md)

Un contrôleur open-source de laveuse de fûts construit sur Arduino Uno. Nettoie les fûts en inox **sans démonter les plongeurs**, en faisant circuler de la soude chaude et de l'acide peracétique via des têtes de lavage.

Ce projet est un fork de [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher) et de [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher). Licence [GNU GPL v3](../../LICENSE).

---

## Ce que ça fait

La machine automatise le cycle complet de nettoyage des fûts :

- Vidange et rinçage initial
- Circulation de détergent (soude à 80 °C, 3 passes)
- Rinçage intermédiaire
- Circulation de désinfectant (acide peracétique, 3 passes)
- Rinçage final + purge et pressurisation CO₂

Un cycle de lavage complet dure environ **5 minutes 30 secondes**. La machine peut laver **2 fûts simultanément**.

📷 *[Photo : vue d'ensemble de la machine]*

---

## Fonctionnalités

- 11 programmes de lavage sélectionnables via un encodeur rotatif
- Afficheur LCD I²C : nom du mode, étape en cours, temps écoulé / temps total
- Mémorisation du dernier mode sélectionné en EEPROM (survit aux coupures de courant)
- LED témoin et buzzer de fin de cycle
- Mode de test des vannes : actionne chaque vanne individuellement pour vérifier le câblage
- Arrêt d'urgence via contacteur (bouton coup-de-poing)

---

## Matériel

| Composant | Détails |
|---|---|
| Contrôleur | Arduino Uno (ATmega328P) |
| Carte de relais | 16 canaux, logique active-low, 12 V |
| Électrovannes | 9 × 12 V |
| Pompe | Novax 20B (ou équivalent) |
| Résistance chauffante | 2500 W (thermoplongeur) |
| Thermostat | STC1000 |
| SSR | 40 A + radiateur + ventilateur 12 V |
| Afficheur | LCD I²C 16×2 |
| Interface | Encodeur rotatif (A1/A2) + bouton action (A3) |

---

## Tester dans le navigateur

Une simulation Wokwi complète est disponible — sans matériel :

**[▶ Ouvrir la simulation](https://wokwi.com/projects/464738080652364801)**

Les électrovannes sont représentées par des LEDs avec leurs labels. Le sketch complet fonctionne sans modification. Note : Wokwi simule le temps plus lentement que la réalité.

---

## Structure du dépôt

```
/
├── kegwasher.ino               — Firmware Arduino (branche main)
├── CHANGELOG.md                — Historique des forks et des modifications
├── CONTRIBUTING.md             — Comment contribuer
├── LICENSE                     — GNU GPL v3
│
├── docs/
│   ├── EN/
│   │   ├── USER_MANUAL.md      — Manuel d'utilisation (EN)
│   │   ├── README_TECHNICAL.md — Choix techniques du firmware
│   │   ├── ELECTRONICS_HOWTO.md— Guide de câblage et d'assemblage
│   │   ├── MAINTENANCE_GUIDE.md— Entretien
│   │   └── GRAFCET_ALL_MODES.md— Diagrammes de séquences (tous modes)
│   └── FR/
│       ├── README_FR.md
│       ├── MANUEL_UTILISATION.md
│       ├── README_TECHNIQUE.md
│       ├── GUIDE_ELECTRONIQUE.md
│       ├── GUIDE_ENTRETIEN.md
│       └── GRAFCET_TOUS_MODES.md
│
├── fritzing/
│   ├── kegwasher.fzz           — Source Fritzing
│   └── kegwasher.png           — Schéma de câblage
│
└── Images/                     — Photos et illustrations
```

---

## Documentation

| Document | EN | FR |
|---|---|---|
| Manuel d'utilisation | [EN](../EN/USER_MANUAL.md) | [FR](MANUEL_UTILISATION.md) |
| README technique | [EN](../EN/README_TECHNICAL.md) | [FR](README_TECHNIQUE.md) |
| Guide d'assemblage électronique | [EN](../EN/ELECTRONICS_HOWTO.md) | [FR](GUIDE_ELECTRONIQUE.md) |
| Guide d'entretien | [EN](../EN/MAINTENANCE_GUIDE.md) | [FR](GUIDE_ENTRETIEN.md) |
| Diagrammes de séquences | [EN](../EN/GRAFCET_ALL_MODES.md) | [FR](GRAFCET_TOUS_MODES.md) |

---

## Historique des forks

| Génération | Auteur | Contributions |
|---|---|---|
| v1 | [vieuxsinge](https://github.com/vieuxsinge/kegwasher) | Projet original : moteur de séquences, modèle bitmask, programmes de lavage, machine à états |
| v2 | [Gummy35](https://github.com/Gummy35/kegwasher) | Séquencement ordonné des actionneurs, affichage LCD alterné, nouveaux modes, pressurisation CO₂ |
| v3 | [ecolab07](https://github.com/ecolab07/kegwasher) | Mode test vannes, transitions minimales, corrections de timing, typage, améliorations hardware |

Voir [CHANGELOG.md](../../CHANGELOG.md) pour l'historique détaillé complet.

---

## Contribuer

Voir [CONTRIBUTING.md](../../CONTRIBUTING.md). Issues et pull requests bienvenus.
