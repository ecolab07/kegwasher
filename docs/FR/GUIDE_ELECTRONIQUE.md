# Guide d'assemblage électronique — Laveuse de fûts

> Ce projet est un fork de [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher)
> et de [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher).
> Licence : GNU GPL v3.

Ce guide décrit l'assemblage du système de contrôle électronique, sous-système par sous-système, dans l'ordre de montage recommandé. Les références et sources des composants sont couvertes dans la [BOM](BOM.md) séparée.

> 📷 *Les emplacements photo sont signalés tout au long du document. Ajoutez vos propres images aux endroits indiqués.*

---

## Prérequis

> 💡 **Avant de commencer le montage**, vous pouvez tester le circuit complet dans le simulateur Wokwi : [https://wokwi.com/projects/464738080652364801](https://wokwi.com/projects/464738080652364801). La simulation inclut l'Arduino, le LCD, la carte de relais, l'encodeur, le bouton, le buzzer et des LEDs représentant chaque électrovanne. C'est un bon moyen de se familiariser avec le comportement du firmware avant de passer au câblage physique. Note : Wokwi simule le temps plus lentement que la réalité — ne pas l'utiliser pour valider les timings.

Un schéma de câblage Fritzing détaillé est fourni dans le dépôt (`fritzing/kegwasher.fzz` et `fritzing/kegwasher.png`). Il reflète la disposition physique réelle des composants dans le boîtier et met en évidence quatre zones : composants déportés (LCD, bouton action), bloc sécurité (arrêt d'urgence + contacteur), prises de puissance (résistance + pompe), zone hydraulique (électrovannes + bus bars). Le fichier source `.fzz` est inclus pour quiconque souhaite modifier ou étendre le schéma.

### Niveaux de compétence
- Les sections **accessibles aux débutants** sont marquées 🟢. Elles ne nécessitent aucune connaissance préalable en électronique — juste la capacité de suivre les instructions attentivement.
- Les sections **intermédiaires** sont marquées 🟡. Une familiarité de base avec le multimètre, le dénudage des fils et la lecture d'un schéma simple est utile.

### Outils nécessaires
- Pince à dénuder
- Tournevis plat et cruciforme
- Multimètre (tension et continuité)
- Outil à sertir (pour les embouts de câble, recommandé)
- Étiqueteuse ou étiquettes adhésives + marqueur

### Règles de sécurité — à lire avant de commencer
> ⚠️ **Le boîtier contient du 230 V CA secteur. Ne jamais intervenir sur le boîtier avec le câble d'alimentation branché.**

- Toujours débrancher la machine du secteur avant d'ouvrir le boîtier ou de toucher un câblage.
- La section 230 V (primaire de l'alimentation, entrée SSR, contacteur d'arrêt d'urgence) doit être câblée avec un câble de section appropriée (2,5 mm² minimum) et maintenue physiquement séparée de la section 12 V / 5 V.
- Utiliser des embouts de câble sur toutes les extrémités de fils souples insérés dans des borniers à vis. Les brins libres peuvent provoquer des courts-circuits.
- En cas de doute, mesurer avant de connecter. Un multimètre en mode continuité ne coûte rien et évite beaucoup de dégâts.

---

## Vue d'ensemble de l'architecture électrique

```
230 V CA secteur
     │
     ├──► Contacteur d'arrêt d'urgence (bouton coup-de-poing)
     │         │
     │         ▼
     │    Bus 230 V CA
     │         │
     │         ├──► Alimentation 12 V CC (120 W)
     │         │         │
     │         │         ├──► Bus 12 V ──► Carte de relais (bobines)
     │         │         │                 Électrovannes
     │         │         │                 Ventilateur SSR
     │         │         │
     │         │         └──► 5 V (depuis carte relais) ──► Arduino Uno
     │         │
     │         └──► SSR (40 A) ──► Thermoplongeur (2500 W)
     │                   ▲
     │              Thermostat STC1000 (signal de commande)
     │
     └──► Thermostat STC1000 (alimentation 230 V)
```

L'électronique de contrôle (Arduino, carte de relais, LCD, encodeur, boutons) fonctionne entièrement en **12 V / 5 V CC** et est complètement isolée de la section 230 V en fonctionnement normal. Les seuls composants 230 V à câbler sont l'alimentation, le SSR, le contacteur et le thermostat.

---

## Sous-système 1 — Préparation du boîtier 🟢

**Ce que c'est** : le boîtier plastique qui abrite toute l'électronique, monté sur le côté du châssis.

**Étapes** :
1. Planifier la disposition interne avant de percer. Disposition suggérée (de haut en bas, de l'avant vers l'arrière) :
   - Façade : LCD, encodeur rotatif, bouton action (LED), bouton d'arrêt d'urgence
   - Intérieur haut : Arduino + shield à bornier à vis, carte de relais
   - Intérieur bas : alimentation 12 V, SSR + radiateur + ventilateur
   - Entrées de câbles : bas du boîtier (éloigne l'humidité des composants)

2. Percer les découpes de la façade pour :
   - Fenêtre LCD (rectangulaire)
   - Axe de l'encodeur rotatif (rond, + méplat pour la vis de blocage du bouton)
   - Bouton action (rond)
   - Bouton d'arrêt d'urgence (rond, standard 22 mm)

3. Percer les entrées de câbles en bas du boîtier. Utiliser des presse-étoupes pour assurer le maintien mécanique et conserver l'indice de protection.

4. Monter les rails DIN ou les entretoises pour la fixation des composants si utilisés.

> 📷 *[Photo : découpes de la façade avant installation des composants]*
> 📷 *[Photo : vue d'ensemble de la disposition interne avec composants positionnés à blanc]*

---

## Sous-système 2 — Alimentation 12 V 🟡

**Ce que c'est** : une alimentation à découpage 230 VCA → 12 VCC (120 W). Elle alimente les bobines de la carte de relais, les électrovannes et le ventilateur du SSR. L'alimentation 5 V de l'Arduino est dérivée du régulateur intégré à la carte de relais.

**Étapes** :
1. Monter l'alimentation dans la partie basse du boîtier.
2. Connecter les bornes d'entrée 230 V CA (L, N, PE) avec un câble de 1,5 mm² depuis le bus de sortie du contacteur. **Ne pas connecter au secteur pour l'instant.**
3. Laisser les bornes de sortie 12 V CC (V+, V−) accessibles pour le sous-système suivant.
4. ✅ **Vérification** : avec un multimètre en tension CC, vérifier que la sortie est à 12 V ± 0,5 V avant de connecter toute charge.

> 📷 *[Photo : alimentation montée et câblée]*

---

## Sous-système 3 — Contacteur d'arrêt d'urgence 🟡

**Ce que c'est** : un contacteur (interrupteur électromécanique) dont la bobine est pilotée par le bouton coup-de-poing d'arrêt d'urgence. Appuyer sur le bouton désactive la bobine, ouvrant les contacts principaux et coupant tout le 230 V de la machine.

Utiliser un contacteur plutôt que de câbler le bouton directement dans la ligne 230 V signifie que le bouton ne commute qu'une bobine à faible courant (~VA), pas la charge totale de la machine. Cela améliore la sécurité et la longévité du bouton dans un environnement humide.

**Étapes** :
1. Monter le contacteur à l'intérieur du boîtier.
2. Connecter l'entrée secteur 230 V CA aux bornes d'entrée ligne du contacteur.
3. Connecter les bornes de sortie charge du contacteur au bus 230 V CA qui alimente l'alimentation et le SSR.
4. Câbler les bornes de bobine du contacteur au bouton d'arrêt d'urgence en façade. Quand le bouton est relâché (état normal), la bobine est alimentée et les contacts sont fermés. Appuyer sur le bouton coupe le circuit de la bobine, ouvrant les contacts.
5. Câbler le bouton d'arrêt d'urgence sur une source 230 V **en amont** du contacteur (le bouton doit rester alimenté même quand le contacteur est ouvert, pour pouvoir le réarmer).

> 📷 *[Photo : détail du câblage du contacteur]*
> 📷 *[Photo : bouton d'arrêt d'urgence en façade]*

> ⚠️ Vérifier la polarité et le marquage des bornes sur le modèle de contacteur utilisé. Les schémas de câblage varient selon les fabricants.

---

## Sous-système 4 — SSR et thermoplongeur 🟡

**Ce que c'est** : un relais statique (SSR) 40 A commute le thermoplongeur dans la cuve de détergent. Le SSR est piloté par le régulateur de température STC1000, qui surveille la température de la cuve via une sonde NTC en inox.

Un relais mécanique ne peut pas commuter fiablement une charge résistive de 2500 W sous 230 V. Le SSR supporte le courant de charge, tandis que la sortie du STC1000 (un relais à faible courant) pilote l'entrée de commande du SSR.

**Radiateur et ventilateur** *(amélioration Ecolab07)* : les SSR dissipent de la chaleur proportionnellement au courant de charge (environ 1–1,5 W par ampère). Sous pleine charge, un SSR 40 A peut dissiper 40–60 W sans radiateur — suffisant pour le détruire. Un radiateur et un ventilateur 12 V sont montés pour maintenir le SSR dans sa température de fonctionnement nominale pendant les longues sessions de chauffage.

**Note** : la résistance d'origine de 3500 W a été remplacée par **2500 W** pour éviter de déclencher le disjoncteur sur des circuits partagés. Le chauffage est plus lent mais reste dans la limite de courant permanent du disjoncteur.

**Étapes** :
1. Monter le SSR sur son radiateur avec de la pâte thermique. Fixer l'ensemble sur la paroi du boîtier (paroi métallique préférable pour la dissipation supplémentaire).
2. Monter le ventilateur 12 V de façon qu'il souffle sur les ailettes du radiateur. Connecter au bus 12 V — le ventilateur tourne en permanence quand la machine est sous tension.
3. Connecter les bornes de charge du SSR (côté CA) en série avec le câble d'alimentation du thermoplongeur (2,5 mm²).
4. Connecter les bornes de commande du SSR (côté CC, typiquement 3–32 V) aux bornes du relais de sortie du STC1000.
5. Câbler le STC1000 :
   - Alimentation 230 V sur l'entrée secteur du thermostat
   - Sonde de température sur l'entrée capteur du thermostat
   - Relais de sortie du thermostat sur l'entrée de commande du SSR (cf. ci-dessus)
6. Installer la sonde de température dans la cuve de détergent via son raccord 1/2" et son kit d'étanchéité.

> 📷 *[Photo : SSR monté sur radiateur avec ventilateur]*
> 📷 *[Photo : STC1000 câblé et monté]*

> ✅ **Vérification** : avec le thermoplongeur immergé et la machine sous tension, régler le STC1000 à une consigne au-dessus de la température ambiante. Vérifier que la LED du SSR s'allume quand le thermostat demande la chauffe, et que l'élément chauffant monte en température.

---

## Sous-système 5 — Carte de relais 🟡

**Ce que c'est** : une carte de relais 16 canaux (9 relais utilisés) qui commute l'alimentation 12 V vers chaque électrovanne et la pompe. Chaque relais est un interrupteur électromécanique avec une bobine 12 V pilotée par un signal logique de l'Arduino. La carte utilise une **logique active-low** : un signal LOW sur une pin de commande active le relais.

**Protection par diodes roue libre** : les électrovannes sont des charges inductives. À la désactivation d'une vanne, un bref pic de surtension (back-EMF) peut détruire les contacts des relais au fil du temps. Une diode roue libre 1N4148 placée aux bornes de chaque vanne (le plus près possible de la sortie du relais) écrête ce pic. **Ne pas omettre ces diodes** — le montage original Vieuxsinge a détruit plusieurs relais avant qu'elles ne soient ajoutées.

**Étapes** :
1. Monter la carte de relais à l'intérieur du boîtier, au-dessus de l'alimentation.
2. Connecter l'entrée d'alimentation 12 V de la carte au bus 12 V.
3. Connecter la sortie 5 V de la carte à la pin 5 V de l'Arduino (cela alimente l'Arduino).
4. Connecter le GND de la carte au GND de l'Arduino.
5. Connecter les pins numériques 2–10 de l'Arduino aux entrées de commande IN1–IN9 de la carte, selon le tableau de correspondance ci-dessous.
6. Pour chacune des 9 sorties de relais actives, câbler les bornes COM et NO (Normalement Ouvert) vers l'électrovanne ou la pompe correspondante, avec une diode roue libre 1N4148 aux bornes de la charge (cathode vers l'alimentation positive).

**Correspondance des pins** :

| Pin Arduino | Canal relais | Actionneur |
|---|---|---|
| 2 | IN1 | Électrovanne air |
| 3 | IN2 | Électrovanne CO₂ |
| 4 | IN3 | Électrovanne eau |
| 5 | IN4 | Vanne entrée détergent |
| 6 | IN5 | Vanne entrée désinfectant |
| 7 | IN6 | Vanne retour détergent |
| 8 | IN7 | Vanne retour désinfectant |
| 9 | IN8 | Vanne de vidange |
| 10 | IN9 | Pompe |

> 📷 *[Photo : carte de relais câblée, montrant les diodes roue libre sur les bornes de sortie]*

---

## Sous-système 6 — Bus bars en sortie des relais 🟢

**Ce que c'est** : des rampes de bus (bus bars) côté sortie des relais, qui regroupent le fil de retour 12 V commun pour des ensembles de vannes. *(Amélioration Ecolab07.)*

Sans bus bars, chaque vanne nécessite son propre fil de retour jusqu'à l'alimentation, produisant un faisceau dense de fils de couleur identique difficile à tracer et à entretenir. Les bus bars permettent un seul fil de retour commun par groupe, avec de courts cavaliers de chaque relais vers la rampe.

**Étapes** :
1. Monter une rampe de bus le long des bornes de sortie des relais.
2. Connecter une extrémité de la rampe au commun de l'alimentation 12 V (V−).
3. Pour chaque sortie de relais, passer un court fil de la borne NO du relais à la rampe (c'est le côté retour commun de la vanne).
4. Passer un fil dédié de la borne COM de chaque relais vers la borne positive de la vanne correspondante.
5. Placer une diode roue libre 1N4148 à chaque point de connexion de vanne (voir sous-système 5).

> 📷 *[Photo : bus bars avec câblage des vannes]*

---

## Sous-système 7 — Arduino et shield à bornier à vis 🟢

**Ce que c'est** : l'Arduino Uno est le cerveau de la machine. Un **shield à bornier à vis** empilé dessus (amélioration Ecolab07) remplace les fragiles connecteurs Dupont du design original par des borniers à vis robustes, et fournit une petite zone de prototypage pour la résistance de la LED.

**Étapes** :
1. Emboîter le shield à bornier à vis sur les connecteurs de l'Arduino Uno.
2. Sur la zone de prototypage du shield, souder une résistance de limitation de courant en série avec le fil signal de la LED (pin 11). Valeur typique : 220–470 Ω selon la tension de seuil de la LED et la luminosité souhaitée.
3. Monter l'ensemble Arduino + shield dans le boîtier avec des entretoises.
4. Connecter les pins de l'Arduino à la carte de relais (voir tableau sous-système 5).
5. Connecter les fils signal restants via les borniers :

| Pin Arduino | Connecté à |
|---|---|
| A0 | Borne positive du buzzer |
| A1 | Pin A de l'encodeur rotatif |
| A2 | Pin B de l'encodeur rotatif |
| A3 | Bouton action (avec pull-up via `INPUT_PULLUP` dans le firmware) |
| A4 | SDA du LCD |
| A5 | SCL du LCD |
| 11 | LED (via résistance sur la zone de prototypage) |

6. Connecter le 5 V et le GND de la carte de relais aux bornes correspondantes de l'Arduino.

> 📷 *[Photo : Arduino avec shield à bornier à vis, vue de dessus montrant la résistance sur la zone de prototypage]*
> 📷 *[Photo : Arduino monté dans le boîtier avec tous les fils connectés]*

---

## Sous-système 8 — Composants de façade 🟢

### Écran LCD
Le LCD est un afficheur I²C 16×2 avec un module backpack qui réduit la connexion à 4 fils : VCC (5 V), GND, SDA (A4), SCL (A5).

1. Monter le LCD derrière la découpe de la façade et fixer avec des vis ou de la colle chaude.
2. Connecter le câble I²C 4 fils au shield de l'Arduino.

> 📷 *[Photo : LCD monté en façade]*

### Encodeur rotatif
L'encodeur rotatif gère la sélection du mode (rotation).

1. Monter l'encodeur dans son trou de façade et fixer avec l'écrou.
2. Fixer le bouton sur l'axe.
3. Connecter les pins A et B de l'encodeur aux pins A1 et A2 de l'Arduino.
4. L'encodeur dispose d'un bouton-poussoir intégré — **note** : dans ce firmware, la fonction bouton utilise un bouton action dédié (voir ci-dessous), pas le bouton intégré à l'encodeur.

> 📷 *[Photo : encodeur rotatif avec bouton, vue façade]*

### Bouton action (avec LED)
Un bouton-poussoir momentané avec LED intégrée. La LED fournit le battement visuel pendant l'exécution des cycles.

1. Monter le bouton dans son trou de façade.
2. Connecter les bornes du bouton à A3 de l'Arduino et au GND. Le firmware utilise `INPUT_PULLUP`, donc aucune résistance de pull-up externe n'est nécessaire.
3. Connecter les bornes de la LED à la pin 11 de l'Arduino (via la résistance sur la zone de prototypage) et au GND.

> 📷 *[Photo : bouton action monté, vue arrière montrant le câblage]*

### Buzzer
Un buzzer piézo passif piloté par `tone()` sur la pin A0.

1. Monter ou coller le buzzer à l'intérieur du boîtier (pas besoin de découpe en façade — le son porte à travers les fentes de ventilation ou la paroi).
2. Connecter la borne positive à A0 de l'Arduino, la borne négative au GND.

---

## Sous-système 9 — Étiquetage des composants 🟢

*(Amélioration Ecolab07.)* Étiqueter chaque composant à l'intérieur du boîtier. Cela rend le diagnostic de pannes et les modifications futures accessibles sans avoir besoin de consulter ce guide.

Étiquettes suggérées :
- Canaux de relais : `RELAIS 1 — AIR`, `RELAIS 2 — CO2`, etc. (suivre le tableau de correspondance du sous-système 5)
- Sections des bus bars : `12V GND — VANNES`
- Bornes de l'alimentation : `12V OUT +`, `12V OUT −`
- SSR : `SSR — RESISTANCE`
- Borniers de l'Arduino : étiqueter chaque borne avec son nom de signal (AIR, CO2, EAU, etc.)

> 📷 *[Photo : intérieur du boîtier étiqueté, vue d'ensemble]*
> 📷 *[Photo : canaux de la carte de relais étiquetés]*

---

## Vérifications finales avant la première mise sous tension

Suivre cette checklist de haut en bas. Ne sauter aucune étape.

**Mécanique**
- [ ] Tous les borniers à vis sont serrés (tirer légèrement chaque fil)
- [ ] Aucun brin de fil nu visible hors des bornes
- [ ] Presse-étoupes serrés
- [ ] Le boîtier se ferme entièrement sans pincer de fils

**Section 230 V** *(multimètre en continuité, machine débranchée)*
- [ ] Pas de continuité entre L et N sur la fiche secteur
- [ ] Pas de continuité entre L/N et PE (terre) sur la fiche secteur
- [ ] Le bouton d'arrêt d'urgence ouvre le conducteur L quand on appuie dessus

**Section 12 V** *(machine sous tension, 230 V connecté, arrêt d'urgence relâché)*
- [ ] 12 V présents aux bornes de sortie de l'alimentation
- [ ] 12 V présents à l'entrée d'alimentation de la carte de relais
- [ ] 5 V présents à la pin 5 V de l'Arduino
- [ ] Toutes les LEDs des relais sont ÉTEINTES (logique active-low : tous les relais doivent être désactivés au démarrage)

**Téléversement du firmware**
- [ ] Connecter l'Arduino au PC via USB
- [ ] Ouvrir `kegwasher.ino` dans l'IDE Arduino
- [ ] Installer les librairies requises : `Bounce2`, `LiquidCrystal_I2C` (rechercher dans le gestionnaire de librairies), `RotaryEncoder`
- [ ] Sélectionner la carte : Arduino Uno, port COM correct
- [ ] Téléverser — aucune erreur de compilation
- [ ] Le LCD affiche `Mode :` et un nom de mode au démarrage
- [ ] L'encodeur rotatif fait défiler les modes
- [ ] Le bouton action lance un cycle

**Test des actionneurs** *(lancer avec toutes les cuves vides)*
- [ ] Sélectionner le mode `Test vannes`
- [ ] Confirmer le prompt `Cuves vides ?` à l'écran
- [ ] Chaque relais claque une fois dans l'ordre — vérifier à l'oreille ou au multimètre sur les bornes des vannes
- [ ] Tous les relais reviennent à l'état désactivé après le test

---

## Diagnostic des problèmes courants

| Symptôme | Cause probable | Action |
|---|---|---|
| Toutes les vannes s'ouvrent au démarrage | Pré-initialisation HIGH absente ou inefficace | Vérifier que le firmware a bien été téléversé ; vérifier la séquence de démarrage de l'Arduino |
| Une vanne ne s'ouvre jamais | Relais ne se déclenche pas, ou problème de câblage vanne | Tester la LED du relais lors de l'étape correspondante ; vérifier la tension aux bornes de la vanne |
| Une vanne ne se ferme jamais | Court-circuit de la diode roue libre | Retirer la diode et mesurer ; remplacer si en court-circuit |
| LCD vide / rétroéclairage seul | Mauvaise adresse I²C | Essayer `0x3F` à la place de `0x27` dans le firmware ; lancer un sketch de scan I²C |
| L'encodeur défile dans le mauvais sens | Pins A/B inversées | Inverser le câblage A1 et A2 |
| SSR en surchauffe | Refroidissement insuffisant | Vérifier que le ventilateur tourne ; vérifier le contact de la pâte thermique sur le radiateur |
| STC1000 ne chauffe pas | Signal de commande SSR absent | Mesurer la tension CC aux bornes de commande du SSR quand la chauffe est demandée |
