# Hydraulics Assembly Guide — Keg Washer

> This guide is largely based on the hydraulic documentation from [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher), with additions specific to this fork.
> License: GNU GPL v3.

---

# Guide d'assemblage hydraulique — Laveuse de fûts

> Ce guide est largement inspiré de la documentation hydraulique de [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher), avec les ajouts spécifiques à ce fork.
> Licence : GNU GPL v3.

---

## Overview / Vue d'ensemble

The hydraulic circuit connects two product tanks (detergent and sanitizer), a mains water inlet, a compressed air inlet, a CO₂ inlet, a pump, two wash heads, and a drain outlet. The circuit is controlled by 9 solenoid valves.

Le circuit hydraulique relie deux cuves produits (détergent et désinfectant), une arrivée d'eau du réseau, une arrivée d'air comprimé, une arrivée de CO₂, une pompe, deux têtes de lavage et une évacuation au caniveau. Le circuit est contrôlé par 9 électrovannes.

📷 *[Photo: full hydraulic circuit overview]*
📷 *[Photo: vue d'ensemble du circuit hydraulique]*

---

## 1. Product tanks / Cuves produits

The product tanks are 50L Bergland stainless steel pots, well-known among homebrewers. Canard rouge sealing kits and barbed fittings provide simple, leak-free tank inlets and outlets.

Les cuves produits sont des marmites Bergland 50L en inox, bien connues des brasseurs amateurs. Des kits d'étanchéité Canard rouge et des raccords cannelés permettent de réaliser très simplement des entrées et sorties pour les produits.

Each tank has:
- 1 bottom outlet → liquid distributor (3/4'' barbed fitting + sealing kit)
- 1 upper inlet → product return from keg (3/4'' barbed fitting + sealing kit)
- 1 × 1'' fitting for the immersion heater (detergent tank only)
- 1 × 1/2'' fitting for the temperature probe (detergent tank only)

Chaque cuve dispose de :
- 1 sortie basse → distributeur de liquides (raccord cannelé 3/4'' + kit d'étanchéité)
- 1 entrée haute → retour produit depuis le fût (raccord cannelé 3/4'' + kit d'étanchéité)
- 1 piquage 1'' pour le thermoplongeur (cuve détergent uniquement)
- 1 piquage 1/2'' pour la sonde de température (cuve détergent uniquement)

Use a hole punch (emporte-pièce) sized for each fitting to make clean, round holes in the tank walls.

Utiliser un emporte-pièce de la taille adaptée à chaque raccord pour réaliser des perçages propres et ronds dans les parois des cuves.

📷 *[Photo: tank fittings and sealing kits]*
📷 *[Photo: raccords et kits d'étanchéité sur les cuves]*

---

## 2. Check valves at tank outlets / Clapets antiretour en sortie de cuves

**ecolab07 addition**: a 3/4'' stainless check valve is installed as close as possible to the outlet of each product tank. This prevents liquid from one tank from back-flowing into the other tank when the pump is running with different inlet valves open.

**Ajout ecolab07** : un clapet antiretour inox 3/4'' est installé au plus près de la sortie de chaque cuve produit. Cela évite qu'un liquide d'une cuve ne remonte dans l'autre cuve lorsque la pompe tourne avec différentes vannes d'entrée ouvertes.

Reference: [Clapet antiretour 3/4'' BSP — Canard rouge](https://microbrassage.com/produits/clapet-anti-retour-3-4-bsp/)

📷 *[Photo: check valve at tank outlet]*
📷 *[Photo: clapet antiretour en sortie de cuve]*

---

## 3. Liquid distributor / Distributeur de liquides

The bottom of each tank and the mains water inlet connect to a liquid distributor made from stainless steel tees: 3 inlets, 1 outlet. Each inlet has a solenoid valve that controls which liquid enters the distributor. The distributor outlet connects to the pump inlet.

Le bas de chaque cuve et l'arrivée d'eau du réseau sont reliés à un distributeur de liquides réalisé à base de tés en inox : 3 entrées et 1 sortie. Sur chaque entrée se trouve une électrovanne qui permet de choisir quel liquide entre dans le distributeur. La sortie du distributeur est reliée à l'entrée de la pompe.

📷 *[Photo: liquid distributor]*
📷 *[Photo: distributeur de liquides]*

---

## 4. Gas distributor / Distributeur de gaz

Like the liquid distributor, the machine has a 2-inlet gas distributor. Solenoid valves control which gas source (air or CO₂) is active. The gas distributor connects to the pump outlet. Intermediate pressure regulators can be fitted on each gas inlet to avoid having to adjust the compressor each time.

Comme pour les liquides, la laveuse est équipée d'un distributeur de gaz à 2 entrées. Les électrovannes permettent d'activer ou non l'arrivée d'air ou de CO₂. Le distributeur de gaz est relié à la sortie de la pompe. Les entrées gaz peuvent être équipées de détendeurs intermédiaires qui évitent de devoir régler le compresseur à chaque fois.

Recommended pressure for both air and CO₂: **2.5 bar**.

Pression recommandée pour l'air et le CO₂ : **2,5 bar**.

📷 *[Photo: gas distributor and pressure regulators]*
📷 *[Photo: distributeur de gaz et détendeurs]*

---

## 5. Check valves at pump / Clapets antiretour sur la pompe

Two check valves are critical:
- **Pump outlet**: prevents gas from flowing back through the pump in reverse when a gas valve is open.
- **Gas inlet** (just before the pump outlet tee): prevents liquid from flowing back into the gas lines.

**Do not skip these valves** — the original Vieuxsinge build destroyed several relay board outputs and pump seals before they were added.

Deux clapets antiretour sont indispensables :
- **Sortie de pompe** : évite que le gaz ne repasse dans la pompe en sens inverse quand une vanne gaz est ouverte.
- **Arrivée gaz** (juste avant le té de sortie de pompe) : évite que le liquide ne remonte dans les lignes de gaz.

**Ne pas omettre ces clapets** — le montage original Vieuxsinge a grillé plusieurs sorties de la carte de relais et des joints de pompe avant qu'ils ne soient ajoutés.

📷 *[Photo: check valves at pump]*
📷 *[Photo: clapets antiretour sur la pompe]*

---

## 6. Wash heads / Têtes de lavage

The pump outlet is split into 2 lines via a tee to feed two wash heads simultaneously. Wash heads are similar to keg couplers but used in reverse, with a larger outlet to allow cleaning liquid to flow through.

La sortie de la pompe est divisée en 2 tuyaux via un té pour alimenter les 2 têtes de lavage simultanément. Les têtes de lavage ressemblent à des têtes de tirage mais utilisées à l'envers, avec une sortie plus grande pour laisser passer le liquide de nettoyage.

The kegs are placed upside down (spear facing down) on the machine's upper support bars. The wash head connects to the spear and the keg body simultaneously.

Les fûts sont posés à l'envers (plongeur vers le bas) sur les barres de support de la partie haute de la machine. La tête de lavage se connecte simultanément sur le plongeur et sur le corps du fût.

📷 *[Photo: wash heads connected to kegs]*
📷 *[Photo: têtes de lavage connectées sur les fûts]*

---

## 7. Product collector / Collecteur de produits

The wash head outlets connect to a product collector: a tee assembly that works in reverse to the distributor — 1 inlet and 3 outlets. Each outlet has a solenoid valve:
- 2 return valves → back into the product tanks (upper inlet)
- 1 drain valve → to the floor drain

Les sorties des têtes de lavage sont reliées à un collecteur de produits. C'est un assemblage de tés qui fonctionne à l'inverse du distributeur : 1 entrée et 3 sorties. Sur chaque sortie se trouve une électrovanne :
- 2 vannes de retour → vers les cuves produits (entrée haute)
- 1 vanne de vidange → vers le caniveau

📷 *[Photo: product collector]*
📷 *[Photo: collecteur de produits]*

---

## 8. Hoses / Tuyaux

| Section | Hose type | Diameter | Notes |
|---|---|---|---|
| Product lines (tanks → pump → wash heads → collector) | Thermoclean 100 | 19 mm | Chemical-resistant, food-safe |
| Wash head connections | Clear crystal | 19 mm | More flexible than Thermoclean — easier keg handling |
| Gas lines (air, CO₂) | Clear crystal | 15 mm | |

| Section | Type de tuyau | Diamètre | Notes |
|---|---|---|---|
| Lignes produit (cuves → pompe → têtes → collecteur) | Thermoclean 100 | 19 mm | Résistant aux produits corrosifs, alimentaire |
| Connexions têtes de lavage | Cristal | 19 mm | Plus souple que le Thermoclean — manipulation des fûts facilitée |
| Lignes gaz (air, CO₂) | Cristal | 15 mm | |

---

## 9. Fittings / Raccords

All fittings are stainless steel and sealed with PTFE tape (teflon) for better resistance to corrosive products.

Tous les raccords sont en inox et étanchéifiés au téflon pour mieux supporter les produits corrosifs.

For drilling tank walls and fitting sealing kits, refer to the [Canard rouge sealing kit instructions](https://www.microbrassage.com/wp-content/uploads/2015/03/notice_kit_etancheite-.pdf).

Pour percer les cuves et fixer les kits d'étanchéité, consulter la [notice du kit d'étanchéité Canard rouge](https://www.microbrassage.com/wp-content/uploads/2015/03/notice_kit_etancheite-.pdf).

---

## 10. Detergent tank heater / Chauffage de la cuve détergent

The detergent tank is equipped with a 2500W ULWD immersion heater (low watt density — gentler on the element and the tank). It is controlled by the CR20 PID temperature controller from Canard rouge, set to maintain 80 °C.

La cuve de détergent est équipée d'un thermoplongeur ULWD 2500W (faible densité de puissance — plus doux pour l'élément et la cuve). Il est contrôlé par le régulateur PID CR20 de Canard rouge, réglé pour maintenir 80 °C.

> ⚠️ A 3500W element was used in the original Vieuxsinge build. This fork uses 2500W to avoid tripping the circuit breaker on shared circuits. Heating is slower but stays within the breaker's sustained current rating.

> ⚠️ Le montage original Vieuxsinge utilise un thermoplongeur de 3500W. Ce fork utilise 2500W pour éviter de déclencher le disjoncteur sur des circuits partagés. Le chauffage est plus lent mais reste dans la limite de courant permanent du disjoncteur.

References / Références :
- [Résistance chauffante V2 2,5kW ULWD + kit de montage — Canard rouge](https://microbrassage.com/produits/resistance-chauffante-v2-25kw-ulwd-avec-kit-de-montage/)
- [PID CR20 pour SSR — Canard rouge](https://microbrassage.com/produits/pid-cr20-pour-ssr/)

📷 *[Photo: immersion heater and CR20 controller]*
📷 *[Photo: thermoplongeur et régulateur CR20]*

---

## Safety notes / Consignes de sécurité

- Always work with the machine unpowered when modifying the hydraulic circuit.
- PTFE tape on all threaded stainless fittings — never leave a fitting dry.
- Check all connections for leaks before the first powered cycle. Run a plain water cycle first.
- Caustic soda (detergent) and peracetic acid (sanitizer) are corrosive. Wear gloves and eye protection when handling concentrates.
- Never mix the two products — they neutralise each other and can react violently at high concentrations.

- Toujours travailler machine hors tension lors de modifications du circuit hydraulique.
- Téflon sur tous les raccords filetés en inox — ne jamais laisser un raccord sec.
- Vérifier toutes les connexions pour détecter les fuites avant le premier cycle sous tension. Lancer d'abord un cycle à l'eau claire.
- La soude (détergent) et l'acide peracétique (désinfectant) sont corrosifs. Porter gants et lunettes lors de la manipulation des concentrés.
- Ne jamais mélanger les deux produits — ils se neutralisent et peuvent réagir violemment à haute concentration.
