# Manuel d'utilisation — Laveuse de fûts automatique

> Ce projet est un fork de [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher)
> et de [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher).
> Licence : GNU GPL v3.

---

## Table des matières

1. [Présentation générale](#1-présentation-générale)
2. [Prérequis avant utilisation](#2-prérequis-avant-utilisation)
3. [Interface utilisateur](#3-interface-utilisateur)
4. [Démarrage et sélection d'un mode](#4-démarrage-et-sélection-dun-mode)
5. [Description des modes](#5-description-des-modes)
6. [Pendant le lavage](#6-pendant-le-lavage)
7. [Fin de cycle](#7-fin-de-cycle)
8. [Annulation d'un cycle](#8-annulation-dun-cycle)
9. [Arrêt d'urgence](#9-arrêt-durgence)
10. [Après utilisation](#10-après-utilisation)
11. [Conseils pratiques](#11-conseils-pratiques)

---

## 1. Présentation générale

La laveuse de fûts automatique permet de nettoyer des fûts en inox **sans démonter les plongeurs**. Les produits de nettoyage sont injectés via des têtes de lavage et circulent à l'intérieur du fût grâce à une pompe.

La machine peut laver **2 fûts simultanément**.

Elle dispose de :
- 2 cuves produits : **détergent** (soude à 2 %, maintenue à 80 °C) et **désinfectant** (acide peracétique à 1 %)
- 1 arrivée d'eau du réseau
- 1 arrivée d'air comprimé
- 1 arrivée de CO₂
- 1 évacuation vers le caniveau

Un cycle de lavage complet dure environ **5 minutes 35 secondes**.

---

## 2. Prérequis avant utilisation

Avant de lancer tout cycle, vérifier les points suivants :

- [ ] Les **cuves produits sont à niveau** (détergent et désinfectant)
- [ ] La **cuve de détergent est à température** (80 °C, vérifier le thermostat STC1000)
- [ ] L'**arrivée d'eau** est ouverte
- [ ] L'**arrivée d'air comprimé** est ouverte (pression recommandée : **2,5 bar**)
- [ ] L'**arrivée de CO₂** est ouverte (pression recommandée : **2,5 bar**)
- [ ] Les **fûts sont connectés** aux têtes de lavage (plongeur vers le bas)
- [ ] L'**évacuation** est dégagée
- [ ] Le **bouton d'arrêt d'urgence** est déverrouillé (tourner pour libérer)

---

## 3. Interface utilisateur

La machine dispose de trois éléments de contrôle sur la façade :

| Élément | Rôle |
|---|---|
| **Encodeur rotatif** | Navigation dans le menu (tourner = changer de mode) |
| **Bouton action** | Valider le mode sélectionné / annuler un cycle en cours |
| **Bouton d'arrêt d'urgence** | Coupure générale immédiate de l'alimentation |

L'**écran LCD** (2 lignes × 16 caractères) affiche :
- Ligne 1 : le nom du mode sélectionné ou l'étape en cours
- Ligne 2 : la progression du cycle (temps écoulé / temps total)

La **LED** sur la façade :
- **Allumée fixe** : en attente de sélection
- **Clignotante** : cycle en cours

Le **buzzer** émet :
- **1 bip** : cycle annulé
- **3 bips** : cycle terminé normalement

---

## 4. Démarrage et sélection d'un mode

1. Mettre la machine sous tension.
2. L'écran affiche `Mode :` sur la première ligne et le nom du dernier mode utilisé sur la seconde *(le dernier mode est mémorisé en cas de coupure de courant)*.
3. **Tourner l'encodeur** pour naviguer dans la liste des modes.
4. **Appuyer sur le bouton action** pour lancer le mode affiché.
5. L'écran affiche `Preparation` le temps que la machine configure les électrovannes, puis le cycle démarre.

---

## 5. Description des modes

### Modes de lavage complets

#### `Lavage + CO2` — 335 s (5 min 35)
Mode de lavage complet recommandé pour un fût ayant contenu de la bière.

| Phase | Durée | Description |
|---|---|---|
| Vidange initiale | 10 s | Évacuation du contenu résiduel vers le caniveau |
| Rinçage initial | 30 s | Passage d'eau + purge air, chasse les résidus grossiers |
| Détergent (×3) | 75 s | Circulation de soude chaude à 80 °C + purge air entre chaque passe |
| Rinçage intermédiaire (×3) | 60 s | Élimination complète des traces de détergent |
| Désinfectant (×3) | 65 s | Circulation d'acide peracétique + purge air entre chaque passe |
| Rinçage final + purge CO₂ | 40 s | Eau + purge CO₂ pour chasser l'oxygène |
| Mise sous pression CO₂ | 10 s | Pressurisation du fût avant remplissage |

✅ **Fût prêt à être rempli directement.**

---

#### `Lavage sans CO2` — 325 s (5 min 25)
Identique au mode précédent mais sans l'étape de pressurisation finale au CO₂. La purge finale est à l'air comprimé.

À utiliser si la mise sous pression CO₂ est faite séparément, ou si le fût n'est pas rempli immédiatement.

---

#### `Detergent seul` — 185 s (3 min 05)
Lavage au détergent et rinçages uniquement, sans étape de désinfection ni CO₂.

À utiliser pour un nettoyage rapide entre deux utilisations rapprochées, ou quand la désinfection sera faite dans un second temps.

---

### Modes de désinfection et pressurisation

#### `Desinf. + CO2` — 190 s (3 min 10)
Désinfection seule suivie d'une purge CO₂ et d'une pressurisation.

À utiliser sur un fût **déjà nettoyé au détergent** dans une session précédente.

---

#### `CO2` — 40 s
Purge CO₂ + pressurisation uniquement, sans produit chimique.

À utiliser pour re-pressuriser un fût propre qui a été laissé ouvert, ou pour chasser l'air résiduel.

---

### Modes de maintenance des cuves produits

#### `Vidange fut` — 70 s (1 min 10)
Vidange du fût via le caniveau, suivie d'une longue purge à l'air pour sécher les lignes.

À utiliser pour vider un fût avant de le stocker ou de le renvoyer.

---

#### `Vidange desinf.` — 200 s (3 min 20)
Vidange de la **cuve désinfectant** à travers le circuit vers le caniveau via la pompe.

À utiliser en fin de session pour vider la cuve de désinfectant, ou pour renouveler la solution.

---

#### `Vidange deter.` — 200 s (3 min 20)
Vidange de la **cuve détergent** à travers le circuit vers le caniveau via la pompe.

À utiliser pour renouveler la solution de détergent ou en fin de saison.

---

#### `Rempl. desinf.` — 120 s (2 min 00)
Remplissage de la **cuve désinfectant** avec de l'eau du réseau via l'électrovanne d'entrée. Le remplissage se fait par pression du réseau uniquement — la pompe n'est pas utilisée.

> ⚠️ S'assurer que la cuve est vide ou à un niveau permettant d'accueillir le volume d'eau avant de lancer ce mode. La vitesse de remplissage dépend entièrement de la pression du réseau.

---

#### `Rempl. deter.` — 120 s (2 min 00)
Remplissage de la **cuve détergent** avec de l'eau du réseau.

> ⚠️ Même précaution que pour le remplissage désinfectant. Penser à ajouter la soude après le remplissage.

---

### Mode de test

#### `Test vannes` — 22 s
Active chaque actionneur **individuellement** pendant 1 seconde, dans l'ordre physique des relais (de gauche à droite et de haut en bas sur la carte).

> ⚠️ **Les cuves doivent être vides avant de lancer ce mode.** L'écran affiche `Cuves vides ?` pendant 3 secondes au démarrage. Attendre ce message et vérifier avant que le test ne commence.

À utiliser pour vérifier le câblage, diagnostiquer une électrovanne silencieuse ou confirmer que la machine est correctement assemblée.

Ordre de déclenchement : désinfectant retour → caniveau → détergent retour → air → CO₂ → désinfectant entrée → eau → détergent entrée → pompe.

---

## 6. Pendant le lavage

- L'écran affiche en ligne 1 le nom de l'étape en cours (alterne avec le nom du mode).
- L'écran affiche en ligne 2 le temps écoulé et le temps total du cycle.
- La LED clignote toutes les 2 secondes.
- **Ne pas débrancher les fûts pendant le cycle.**

---

## 7. Fin de cycle

Lorsque toutes les étapes sont terminées :
1. Toutes les électrovannes se ferment et la pompe s'arrête.
2. La LED s'éteint.
3. L'écran affiche `Termine`.
4. Le buzzer émet **3 bips**.
5. La machine revient automatiquement au menu de sélection.

---

## 8. Annulation d'un cycle

Appuyer sur le **bouton action** pendant un cycle en cours pour l'annuler immédiatement :
1. Toutes les électrovannes se ferment et la pompe s'arrête.
2. L'écran affiche `Annule`.
3. Le buzzer émet **1 bip**.
4. La machine revient au menu de sélection.

> ⚠️ En cas d'annulation en cours de cycle, les fûts et les lignes peuvent contenir des résidus de produit. Relancer un cycle de rinçage avant de remplir les fûts.

---

## 9. Arrêt d'urgence

Le bouton **coup-de-poing** sur la façade coupe l'alimentation générale via un contacteur.

Il agit sur **toute la machine** (pompe, électrovannes, Arduino).

Pour réarmer : **tourner le bouton** dans le sens indiqué jusqu'au déclic.

> ⚠️ Après un arrêt d'urgence, toutes les électrovannes se trouvent dans leur état de repos (fermé = sécurité). Vérifier l'état du circuit avant de remettre sous tension.

---

## 10. Après utilisation

- Vérifier que les cuves produits sont à niveau pour la prochaine session.
- Si la session est terminée pour la journée, laisser les cuves produits en place (elles sont conçues pour rester en circuit).
- Fermer les arrivées d'eau, d'air et de CO₂ si la machine n'est pas utilisée pendant une longue période.
- Rincer et sécher les têtes de lavage.

---

## 11. Conseils pratiques

- **Optimisation du temps** : le lavage (5 min) peut être réalisé en parallèle du remplissage des fûts déjà propres. C'est tout l'intérêt de l'automatisation.
- **Température du détergent** : attendre que le thermostat STC1000 ait atteint 80 °C avant de lancer un cycle de lavage complet. Un détergent froid est moins efficace. Un touilleur pourrait aider à homogénéiser la température de la cuve, mais en pratique la turbulence générée par la pompe de circulation pendant les cycles de détergent suffit à maintenir la solution bien mélangée.
- **Renouvellement des produits** : utiliser les modes `Vidange` et `Rempl.` pour renouveler les solutions en fin de saison ou selon la fréquence recommandée par le fournisseur des produits.
- **Vérification des têtes de lavage** : s'assurer que les têtes sont bien encliquetées sur les plongeurs avant chaque cycle. Une tête mal connectée entraîne une fuite et un mauvais lavage.
