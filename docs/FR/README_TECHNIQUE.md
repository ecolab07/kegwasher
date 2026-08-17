# kegwasher — README Technique

> Ce projet est un fork de [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher)
> et de [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher).
> Licence : GNU GPL v3.

Ce document couvre les choix techniques effectués au fil des trois générations du projet. Pour les instructions d'utilisation, voir [MANUEL_UTILISATION.md](MANUEL_UTILISATION.md).

---

## Table des matières

1. [Vue d'ensemble de l'architecture](#1-vue-densemble-de-larchitecture)
2. [Moteur de séquences par étapes](#2-moteur-de-séquences-par-étapes)
3. [Modèle de contrôle des actionneurs](#3-modèle-de-contrôle-des-actionneurs)
4. [Séquencement ordonné ouverture/fermeture — Gummy35 + Ecolab07](#4-séquencement-ordonné-ouverturefermeture--gummy35--ecolab07)
5. [Pré-initialisation des pins à HIGH — Ecolab07](#5-pré-initialisation-des-pins-à-high--ecolab07)
6. [Machine à états](#6-machine-à-états)
7. [Affichage LCD alterné — Gummy35 + Ecolab07](#7-affichage-lcd-alterné--gummy35--ecolab07)
8. [Encodeur rotatif : protection contre les valeurs négatives — Ecolab07](#8-encodeur-rotatif--protection-contre-les-valeurs-négatives--ecolab07)
9. [Persistance du mode sélectionné en EEPROM](#9-persistance-du-mode-sélectionné-en-eeprom)
10. [Mode test des actionneurs — Ecolab07](#10-mode-test-des-actionneurs--ecolab07)
11. [Catalogue des modes de lavage](#11-catalogue-des-modes-de-lavage)
12. [Notes hardware](#12-notes-hardware)
13. [Dépendances](#13-dépendances)
14. [Simulation Wokwi](#14-simulation-wokwi)
15. [Schéma de câblage (Fritzing)](#15-schéma-de-câblage-fritzing)
16. [Limitations connues et travaux futurs](#16-limitations-connues-et-travaux-futurs)

---

## 1. Vue d'ensemble de l'architecture

Le firmware est un unique fichier `.ino` structuré autour de trois couches indépendantes :

```
┌─────────────────────────────────┐
│  Table des modes (MODES[])      │  Modes nommés → tableaux d'étapes
├─────────────────────────────────┤
│  Moteur de séquences            │  Étapes → configs actionneurs + timing
├─────────────────────────────────┤
│  Couche de contrôle actionneurs │  Bitmasks → écritures GPIO individuelles
└─────────────────────────────────┘
```

Ajouter un nouveau programme de lavage ne nécessite qu'un nouveau tableau `step_t` et une entrée dans `MODES[]`. Aucune modification de la logique de contrôle n'est requise.

---

## 2. Moteur de séquences par étapes

Chaque programme de lavage est exprimé comme un tableau de structs `step_t` :

```c
typedef struct step_s {
  unsigned int  config;   // masque de bits des actionneurs à activer
  unsigned long duration; // durée de maintien de cette configuration (secondes)
} step_t;
```

Le tableau est terminé par la valeur sentinelle `{CONFIG_END, 0}` (où `CONFIG_END == 0`), ce qui permet à `run_update()` de détecter la fin de séquence sans paramètre de longueur.

À l'exécution, `step_set(index)` applique la configuration des vannes via `controls_set()` (bloquant), puis enregistre `step_start_time` après la fin des transitions. `run_update()` est appelé à chaque itération de `loop()` et passe à l'étape suivante dès que `seconds() - step_start_time >= step.duration`.

Enregistrer `step_start_time` après les transitions garantit que la durée déclarée de chaque étape est mesurée à partir du moment où les vannes sont réellement dans leur état cible, et non avant le délai bloquant. Le même principe s'applique à `mode_start_time`, enregistré après `step_set(0)` dans `run()`.

L'utilisation des secondes horloge (via `millis() / 1000`) plutôt qu'un compteur rend le timing des étapes indépendant du temps d'exécution de `loop()`. Toutes les variables de temps utilisent `unsigned long` (32 bits sur AVR), cohérent avec `millis()` et à l'abri du débordement 16 bits qui affecterait `int` après ~9 heures.

---

## 3. Modèle de contrôle des actionneurs

Chacune des 9 sorties physiques se voit attribuer un bit unique en puissance de deux :

```c
#define CTRL_WATER          0b000000001
#define CTRL_CLEANER_IN     0b000000010
#define CTRL_SANITIZER_IN   0b000000100
#define CTRL_AIR            0b000001000
#define CTRL_CO2            0b000010000
#define CTRL_DRAIN          0b000100000
#define CTRL_CLEANER_OUT    0b001000000
#define CTRL_SANITIZER_OUT  0b010000000
#define CTRL_PUMP           0b100000000
```

Les configurations composites pour les séquences de lavage sont construites par OU de bits individuels :

```c
#define CONFIG_CLEAN  (CTRL_PUMP + CTRL_CLEANER_IN + CTRL_CLEANER_OUT)
```

Cette représentation est compacte, lisible et facilement extensible. Ajouter un 10e actionneur ne nécessite qu'un nouveau bit et une nouvelle pin — aucune modification structurelle.

Le mode test utilise délibérément des bits `CTRL_*` bruts (un actionneur à la fois) plutôt que des valeurs composites `CONFIG_*`, ce qui le rend utile pour diagnostiquer le comportement de chaque vanne individuellement.

---

## 4. Séquencement ordonné ouverture/fermeture — *Gummy35* + *Ecolab07*

Le firmware original commutait tous les actionneurs simultanément sans délai, avec des valeurs `HIGH`/`LOW` brutes. Gummy35 a entièrement refactorisé `controls_set_state()`, introduisant trois changements.

**Constantes logiques nommées** remplaçant les `HIGH`/`LOW` bruts :
```c
#define VALVE_CLOSE HIGH
#define VALVE_OPEN  LOW
```
Cela rend la logique active-low des relais explicite à chaque point d'appel.

**Ordre par direction** — les actionneurs sont désormais ouverts et fermés dans un ordre soigneusement choisi :

Ordre de fermeture (sources de pression d'abord, puis sorties) : pompe → eau/CO₂/air → entrées liquide → voies de retour → caniveau (en dernier, pour que la pression résiduelle puisse s'échapper).

Ordre d'ouverture (chemin libre avant la pression) : caniveau/retours → entrées liquide → pompe (aspire le liquide, réduisant la pression d'entrée) → air → CO₂ → eau (en dernier — la contre-pression de la pompe empêche le reflux dans les cuves).

Cet ordre prévient les pics de pression qui piègent le liquide dans les lignes lors de la fermeture, le reflux d'eau ou de produits chimiques dans les lignes CO₂ ou air lors de l'ouverture, et la cavitation de la pompe.

**Délai inter-actionneur à la fermeture** augmenté de 0 ms à 200 ms, correspondant au délai d'ouverture déjà présent dans l'original :

```c
// avant (Vieuxsinge)
controls_set_state(~config, HIGH, 0);   // fermeture : sans délai
controls_set_state(config,  LOW,  200); // ouverture : 200 ms

// après (Gummy35)
controls_set_state(~config, VALVE_CLOSE, 200); // fermeture : 200 ms
controls_set_state(config,  VALVE_OPEN,  200); // ouverture : 200 ms
```

**Aussi modifié par Gummy35** : `CONFIG_FILL_SANITIZER` et `CONFIG_FILL_CLEANER` avaient `CTRL_PUMP` supprimé (remplissage par pression du réseau uniquement). Cela a été rétabli dans le fork ecolab07 — la pompe est nécessaire pour faire circuler l'eau à travers le fût et la renvoyer dans la cuve via les vannes de retour (`CTRL_SANITIZER_OUT` / `CTRL_CLEANER_OUT`). Les vannes d'entrée (`CTRL_SANITIZER_IN` / `CTRL_CLEANER_IN`) sont orientées dans le mauvais sens pour ce chemin d'écoulement et ne sont pas utilisées lors du remplissage.

**Ecolab07 — logique de transition minimale** : `controls_set_state()` a été découpée en `close_actuators()` et `open_actuators()`, et `controls_set()` a été réécrite pour ne toucher que les actionneurs qui changent réellement d'état :

```c
unsigned int previous_config = 0;  // suit la configuration active

void controls_set(unsigned int config)
{
  unsigned int to_close = previous_config & ~config;
  unsigned int to_open  = config & ~previous_config;

  close_actuators(to_close);
  open_actuators(to_open);

  previous_config = config;
}
```

`previous_config & ~config` isole les actionneurs qui étaient ouverts et ne sont plus nécessaires. `config & ~previous_config` isole les actionneurs nécessaires qui n'étaient pas encore ouverts. Les actionneurs présents dans les deux configurations ne sont pas touchés. Trois bénéfices :

- **Courant d'appel** : seuls les relais qui changent vraiment d'état commutent, jamais les 9 simultanément.
- **Précision du timing** : le délai de 200 ms ne s'applique qu'aux actionneurs qui bougent, donc les transitions entre configurations similaires (ex. `CONFIG_CLEAN` → `CONFIG_CLEAN_PURGE`, qui partagent `CTRL_CLEANER_OUT`) sont plus rapides.
- **Sécurité de `CONFIG_WARNING` / `CONFIG_WAIT` par conception** : ces pseudo-configs ont les bits 10 et 11, hors de la plage des 9 bits d'actionneurs réels. `to_close` et `to_open` masquent naturellement ces bits — aucun actionneur n'est jamais déclenché, quelle que soit la valeur de `previous_config`.

---

## 5. Pré-initialisation des pins à HIGH — *Ecolab07*

La carte relais utilise une **logique active-low** : un signal LOW sur une pin de contrôle active le relais et ouvre la vanne. Les pins GPIO de l'Arduino sont en haute impédance par défaut au démarrage, ce que la carte relais tire à LOW — activant brièvement tous les relais avant l'appel de `pinMode(OUTPUT)` dans `setup()`.

Cela provoque un pic de courant d'appel non contrôlé (9 relais commutant simultanément) et, plus grave, une ouverture momentanée de toutes les vannes.

Le correctif consiste à écrire `HIGH` sur chaque pin *avant* d'appeler `pinMode()` :

```c
// Pré-forcer HIGH avant de configurer en sortie pour éviter
// le glitch LOW bref qui activerait tous les relais au démarrage
digitalWrite(PIN_VALVE_AIR, HIGH);
// ... toutes les autres pins vannes et pompe ...
pinMode(PIN_VALVE_AIR, OUTPUT);
// ...
```

Sur les Arduino AVR, `digitalWrite()` sur une pin encore en mode entrée écrit dans le registre de pull-up, ce qui ne produit pas de glitch LOW. La transition vers le mode `OUTPUT` part donc d'un niveau HIGH engagé, maintenant tous les relais désactivés.

---

## 6. Machine à états

La boucle principale dispatch sur un enum `state_t` :

```
STATE_SELECT → STATE_SELECT_UPDATE ──────────────────────┐
                     │ (appui bouton)                     │
                     ▼                                    │
               STATE_RUN                                  │
                     │                                    │
                     ▼                                    │
             STATE_RUN_UPDATE ──── (appui bouton) ──► STATE_CANCEL ─┐
                     │ (CONFIG_END)                                  │
                     ▼                                               │
             STATE_TERMINATE ──────────────────────────────────────►┘
                                                    (tous → STATE_SELECT)
```

Chaque état a une fonction gestionnaire dédiée. Les transitions sont écrites comme des affectations à la variable globale `state`, sur laquelle la prochaine itération de `loop()` va dispatcher. Cela maintient chaque gestionnaire petit et à responsabilité unique.

---

## 7. Affichage LCD alterné — *Gummy35* + *Ecolab07*

Pendant un cycle en cours, la ligne 1 du LCD alterne entre deux informations sur une cadence `LED_BLINK_PERIOD` (2 s) :

- **Première moitié de la période** : le label de l'étape en cours — ex. `Detergent`, `Purge air`
- **Seconde moitié de la période** : le nom du mode — ex. `Lavage + CO2`

La LED clignote en synchronisation avec cette alternance, fournissant un battement visible qui confirme aussi que l'Arduino tourne toujours.

```c
if( mode_running_time % LED_BLINK_PERIOD < LED_BLINK_PERIOD/2 ) {
    digitalWrite(PIN_LED, HIGH);
    lcd.setCursor(0, 0);
    lcd_printf(resolve_label(MODES[mode].steps[step].config));
}
else {
    digitalWrite(PIN_LED, LOW);
    lcd.setCursor(0, 0);
    lcd_printf(MODES[mode].name);
}
```

**Gummy35** a introduit cet affichage alterné et résolvait les labels via une variable globale `config_label` écrite dans `controls_set_state()`. **Ecolab07** a remplacé cela par `resolve_label()`, une fonction pure retournant un `const char*` vers un littéral en Flash sans effets de bord. La variable globale est supprimée — le label est résolu à la demande directement depuis la config de l'étape courante.

---

## 8. Encodeur rotatif : protection contre les valeurs négatives — *Ecolab07*

La librairie `RotaryEncoder` retourne une position entière signée qui décrémente en dessous de zéro quand l'encodeur est tourné dans le sens anti-horaire au-delà de la position 0. Une opération modulo naïve en C produit des résultats négatifs pour des opérandes négatifs (le signe suit le dividende), ce qui produirait un index invalide dans `MODES[]`.

**Première approche** — ajouter `MODES_NUMBER` avant le modulo :
```c
new_mode = (pos + MODES_NUMBER) % MODES_NUMBER;
```
Fonctionne correctement pour un tour complet anti-horaire (ex. `pos = -3` avec `MODES_NUMBER = 11` donne `8` ✅), mais échoue silencieusement quand `pos` est plus négatif que `-MODES_NUMBER` (ex. `pos = -14` donne `-3` ❌, car `(-14 + 11) % 11 = -3` en C).

**Approche actuelle** — double modulo :
```c
new_mode = ((pos % MODES_NUMBER) + MODES_NUMBER) % MODES_NUMBER;
```
`pos % MODES_NUMBER` réduit d'abord toute valeur dans `(-MODES_NUMBER, MODES_NUMBER)`, puis l'addition de `MODES_NUMBER` la rend positive, et le dernier `% MODES_NUMBER` la ramène dans `[0, MODES_NUMBER - 1]`. Correct pour toute valeur de `pos`, quel que soit le nombre de tours anti-horaires effectués.

---

## 9. Persistance du mode sélectionné en EEPROM

L'index du dernier mode sélectionné est écrit à l'adresse EEPROM `0` au lancement d'un cycle (uniquement si la valeur a changé, pour limiter l'usure en écriture) :

```c
int saved_mode = EEPROM.read(EEPROM_ADDRESS_MODE);
if( mode != saved_mode ) {
    EEPROM.write(EEPROM_ADDRESS_MODE, mode);
}
```

Au démarrage, la valeur stockée est lue et clampée à la plage valide :

```c
mode = constrain(mode, 0, MODES_NUMBER - 1);
```

Le clampage gère le cas où l'EEPROM contient `0xFF` (état effacé) ou un index obsolète d'une version firmware avec moins de modes.

---

## 10. Mode test des actionneurs — *Ecolab07*

Le mode test actionne chacun des 9 actionneurs individuellement pendant 1 seconde, dans l'ordre physique de la carte de relais, avec un intervalle de 1 seconde entre chaque impulsion.

**Intention de conception** : le mode test est implémenté entièrement comme un tableau `step_t` (`STEPS_TEST_ACTUATORS`), utilisant le même moteur de séquences que tous les autres modes. Aucun code spécial n'a été ajouté à la couche de contrôle.

Chaque étape utilise un bit `CTRL_*` brut plutôt qu'une valeur composite `CONFIG_*`, donc exactement un relais claque par étape. Cela permet de vérifier chaque vanne facilement à l'oreille ou en observant le circuit.

Deux pseudo-configurations spéciales sont introduites exclusivement pour ce mode :

| Constante | Bit | Rôle |
|---|---|---|
| `CONFIG_WARNING` | bit 10 | Affiche un prompt de sécurité (`Cuves vides ?`) avant le premier déclenchement de relais. Aucun actionneur n'est activé. |
| `CONFIG_WAIT`    | bit 11 | Étape d'attente entre les impulsions : `controls_set(CONFIG_WAIT)` ferme tout (les bits 0–9 sont tous à zéro dans le masque), produisant un état OFF propre entre chaque actionnement individuel. |

Les deux bits sont hors de la plage 9 bits des actionneurs réels, ils ne peuvent donc pas entrer accidentellement en collision avec une sortie physique.

---

## 11. Catalogue des modes de lavage

| Mode | Tableau | Durée | Gummy35 | Ecolab07 |
|---|---|---|---|---|
| `Lavage + CO2` | `STEPS_WASH_KEG_PRESSURIZE` | 330 s | ✅ étape CO₂ | |
| `Lavage sans CO2` | `STEPS_WASH_KEG` | 320 s | | |
| `Detergent seul` | `STEPS_DETER_KEG` | 180 s | ✅ ajouté | |
| `CO2` | `STEPS_KEG_PRESSURIZE` | 50 s | ✅ ajouté | |
| `Desinf. + CO2` | `STEPS_SANITIZE_KEG_PRESSURIZE` | 190 s | ✅ ajouté | |
| `Vidange fut` | `STEPS_DRAIN_KEG` | 70 s | | |
| `Vidange desinf.` | `STEPS_DRAIN_SANITIZER` | 200 s | | |
| `Vidange deter.` | `STEPS_DRAIN_CLEANER` | 200 s | | |
| `Rempl. desinf.` | `STEPS_FILL_SANITIZER` | 120 s | | |
| `Rempl. deter.` | `STEPS_FILL_CLEANER` | 120 s | | |
| `Test vannes` | `STEPS_TEST_ACTUATORS` | 24 s | | ✅ ajouté |

---

## 12. Notes hardware

### Carte de relais — logique active-low
La carte de relais 16 canaux utilise des entrées active-low. Toutes les pins de contrôle sont pré-forcées à HIGH au démarrage (voir §5) pour éviter tout actionnement involontaire.

### Protection par diodes roue libre
Les électrovannes sont des charges inductives. Sans protection, le pic de surtension à la désactivation d'une vanne peut détruire les contacts des relais. Une diode roue libre 1N4148 est placée aux bornes de chaque vanne, le plus près possible de la sortie du relais.

### SSR pour la résistance chauffante du détergent
La résistance chauffante (thermoplongeur) dans la cuve de détergent est commutée par un relais statique (SSR) 40 A, piloté par le thermostat STC1000. L'élément de 3500 W d'origine a été remplacé par **2500 W** pour éviter de déclencher le disjoncteur sur des circuits partagés — le chauffage est plus lent mais reste dans la limite de courant permanent du disjoncteur. Le SSR est équipé d'un **radiateur et d'un ventilateur 12 V** (amélioration Ecolab07) pour éviter l'emballement thermique lors des longues sessions de chauffage.

### Arrêt d'urgence via contacteur
La conception originale connectait le bouton d'arrêt d'urgence directement dans le circuit de charge. Ecolab07 a remplacé cela par un **contacteur** piloté par le bouton coup-de-poing, de sorte que le bouton ne commute qu'une bobine à faible courant plutôt que le courant de charge total de la machine. Cela améliore la longévité du bouton et est plus sûr dans un environnement humide.

### Shield à bornier à vis pour Arduino
Ecolab07 a ajouté un **shield à bornier à vis** sur l'Arduino, équipé d'une zone de prototypage intégrée. Cela permet :
- de monter proprement la résistance de limitation de courant de la LED sur la platine
- de terminer tous les câbles externes sur des borniers à vis plutôt que sur des connecteurs Dupont, améliorant la fiabilité mécanique dans un environnement soumis aux vibrations et à l'humidité.

### Rampes de bus en sortie des relais
Ecolab07 a ajouté des **rampes de bus (bus bars)** côté sortie des relais pour simplifier le câblage des électrovannes. Chaque bus regroupe le fil de retour 12 V commun pour un ensemble de vannes, réduisant la complexité du câblage point à point et améliorant la lisibilité du boîtier.

### Étiquetage des composants
Tous les composants à l'intérieur du boîtier sont identifiés par des étiquettes adhésives. Cela rend le diagnostic de pannes et la maintenance accessibles sans avoir besoin de consulter le schéma de câblage.

---

## 13. Dépendances

| Librairie | Version testée | Utilisation |
|---|---|---|
| `Bounce2` | any | Anti-rebond du bouton |
| `LiquidCrystal_I2C` | 1.1.2 | Pilote LCD I²C |
| `RotaryEncoder` | any | Lecture de l'encodeur rotatif |

**`LiquidCrystal_I2C`** — auteur : Frank de Brabander, mainteneur : Marco Schwartz, dépôt : [github.com/johnrickman/LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C). Installer via le gestionnaire de librairies de l'IDE Arduino en recherchant `LiquidCrystal I2C` — cela installe la version 1.1.2, utilisée dans ce projet et fonctionnelle sans modification. Il existe plusieurs autres forks de cette librairie sous des noms similaires ; utiliser le mauvais peut provoquer des erreurs de compilation ou un comportement d'affichage incorrect.

---

## 14. Simulation Wokwi

Une simulation complète du circuit est disponible à :
**[https://wokwi.com/projects/464738080652364801](https://wokwi.com/projects/464738080652364801)**

La simulation inclut l'Arduino Uno, l'afficheur LCD I²C, la carte de relais (logique active-low), la LED témoin, le buzzer, l'encodeur rotatif et le bouton action. Les électrovannes sont représentées par des LEDs avec leurs labels, permettant la vérification visuelle de chaque séquence d'actionnement.

Le sketch complet fonctionne sans modification dans le simulateur.

**Limitations connues de Wokwi par rapport au hardware réel :**
- Les secondes s'écoulent plus lentement que dans la réalité — Wokwi ne simule pas le temps horloge à vitesse 1:1, donc les durées de cycle paraissent plus longues que déclaré.
- La logique active-low de la carte de relais est simulée avec une polarité inversée — vérifier que les defines `VALVE_OPEN`/`VALVE_CLOSE` correspondent à votre carte relais physique avant de flasher sur le hardware.

---

## 15. Schéma de câblage (Fritzing)

Un schéma de câblage Fritzing détaillé est fourni dans le dépôt (`fritzing/kegwasher.fzz` et `fritzing/kegwasher.png`). Le schéma reflète la disposition physique réelle des composants dans le boîtier et met en évidence quatre zones distinctes :

- **Composants déportés** : afficheur LCD et bouton action câblés sur la façade
- **Bloc sécurité** : bouton d'arrêt d'urgence coup-de-poing et contacteur
- **Prises de puissance** : points de connexion du thermoplongeur et de la pompe
- **Zone hydraulique** : câblage des électrovannes et bus bars en sortie des relais

Le fichier source `.fzz` est inclus pour quiconque souhaite modifier ou étendre le schéma.

---

## 16. Limitations connues et travaux futurs

### Temps affiché vs temps réel
Le délai de 200 ms inter-actionneur dans `close_actuators()` et `open_actuators()` est un appel `delay()` bloquant. Pendant les transitions, `loop()` ne tourne pas, donc les secondes affichées s'accumulent légèrement plus lentement que les secondes réelles. Le surcoût par transition est proportionnel au nombre d'actionneurs qui changent d'état (0–9 relais × 200 ms). Pour les modes les plus longs, cela ajoute environ 15 s de dérive sur un cycle de 5 min 35 s.

La branche `feature/nonblocking-timers` résout ce point en mesurant la durée des transitions avec `millis()` et en la soustrayant du temps affiché.

### Polling de l'encodeur et du bouton
L'encodeur et le bouton sont pollés dans la boucle principale (`menuselect.tick()`, `buttonAction.update()`). Pendant les transitions bloquantes, les appuis sur le bouton peuvent être manqués s'ils se produisent dans une fenêtre de 200 ms.

La branche `feature/rotary-interrupts` résout ce point en migrant la gestion de l'encodeur et du bouton vers des interruptions Pin Change (PCINT), rendant la détection des entrées indépendante du timing de la boucle.
