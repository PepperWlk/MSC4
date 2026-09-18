# Cahier des besoins — Jeu Sandbox Désert (inspiré de Kenshi)
### Moteur : Unreal Engine

## 1. Concept

Jeu de survie sandbox en monde ouvert désertique, **centré sur un seul personnage** (contrairement à Kenshi, pas de gestion d'escouade/base). L'accent est mis sur la **liberté du joueur** : pas de scénario linéaire imposé, le joueur choisit son style de jeu (combattant, marchand itinérant, chasseur, pillard, ermite...) et progresse par ses actions.

## A. Décisions actées

| Sujet | Décision |
|---|---|
| Moteur | **Unreal Engine** (UE5) |
| Système de dégâts | **Score de vie classique** (pas de dégâts localisés par membre) |
| Premier chantier | **Carte désertique** (dunes + zones réservées pour points d'intérêt) |

---

## B. Besoins fonctionnels

### A. Survie
- Jauges de faim, soif, fatigue, température corporelle
- Chaleur le jour (risque de coup de chaleur), froid la nuit
- Nécessité de trouver eau, nourriture, ombre/abri
- Blessures et états négatifs (déshydratation, infection, épuisement)
- Sommeil / repos pour récupérer stamina et soigner

### B. Exploration
- Monde désertique (dunes, oasis, canyons rocheux, ruines) — génération procédurale
- Cycle jour/nuit influant sur température, visibilité et rencontres
- Météo dynamique (tempêtes de sable réduisant la visibilité et augmentant le danger)
- Points d'intérêt : villages (futurs), campements de bandits, ruines, oasis, puits

### C. Rencontres (cœur du gameplay)
- **Bandits** : embuscades, groupes, butin à la clé
- **Animaux** : certains hostiles/prédateurs, certains chassables (ressources), éventuellement apprivoisables/montables
- **Marchands** : caravanes itinérantes, achat/vente, possibilité de les escorter contre rémunération ou de les attaquer
- Génération des rencontres selon la zone, l'heure et le niveau de danger

### D. Combat
- Corps à corps et/ou à distance (arc, armes de jet)
- Score de vie unique, dégâts modulés par armure/résistances (pas de localisation par membre)
- Fuite et discrétion comme alternatives valables au combat frontal

### E. Économie / Inventaire
- Inventaire avec poids/encombrement
- Équipement : armes, armures, vêtements adaptés au désert (protection chaleur/froid)
- Craft basique : réparation, fabrication d'objets de survie
- Commerce avec les marchands, prix variables selon réputation/rareté

### F. Progression
- Pas de classes figées : compétences qui progressent par la pratique (façon Kenshi/Skyrim)
- Réputation avec les factions (bandits, marchands, villages)

### G. Intelligence artificielle
- Animaux : fuite, chasse, comportement territorial ou de meute
- Bandits : patrouille, embuscade, poursuite, retraite si en infériorité
- Marchands : déplacement entre points, fuite ou appel à l'aide si menacés

---

## 4. Besoins non-fonctionnels
- Moteur : Unreal Engine 5
- Plateforme cible : PC en priorité
- Monde ouvert : **World Partition** pour le streaming de zones à grande échelle 
- Système de sauvegarde (état du monde + du joueur)

---

## 2. Documentation Technique

### 2.1 Principales Classes, Blueprints et Composants

L'architecture repose sur une approche modulaire combinant des composants d'acteurs et des conteneurs de données.

- **BP_InventoryComponent (`ActorComponent`)** : Composant gérant la logique globale d'inventaire, l'ajout/retrait d'objets et la communication avec l'interface utilisateur.
- **PDA_ItemDefinition (`PrimaryDataAsset`)** : Structure de données fondamentale définissant les propriétés des objets (nom, icône, type, caractéristiques, poids).
- **ST_InventoryItem (`UserDefinedStruct`)** : Structure répercutant l'état d'un objet en jeu (référence au Data Asset, quantité, état).
- **BP_InteractableObjects (`Actor`)** : Classe parente pour tout élément interactif présent dans le monde.
- **BP_LootableItem (`BP_InteractableObjects`)** : Classe dérivée spécialisée dans la prise en charge des objets ramassables et la liaison avec le composant d'inventaire.
- **BPI_Interactable (`BlueprintInterface`)** : Interface assurant le découplage entre l'acteur effectuant l'interaction et l'objet cible.

---

### 2.2 Fonctionnement du Système d'Interaction

Le système d'interaction est entièrement basé sur une **Blueprint Interface** (`BPI_Interactable`), garantissant qu'aucun couplage fort n'existe entre le personnage et les objets interactifs.

1. **Détection** : Le joueur effectue un *Line Trace* ou *Sphere Trace* depuis le personnage/caméra.
2. **Vérification** : Le système vérifie si l'acteur touché implémente l'interface `BPI_Interactable`.
3. **Exécution** : Lors de l'appui sur la touche d'action, la fonction `Interact` transmise via l'interface est appelée sur l'acteur cible.
4. **Traitement** : L'objet cible (`BP_LootableItem` ou autre) exécute sa propre logique (ex: transfert de données vers le `BP_InventoryComponent` du joueur puis destruction de l'acteur).

---

### 2.3 Principaux Choix Techniques

1. **Architecture orientée composants (Component-Driven Architecture)** : Utilisation de `BP_InventoryComponent` pour garantir la réutilisabilité sur d'autres entités (ex: coffres, PNJ) sans dupliquer le code dans les personnages.
2. **UDataAsset pour les définitions d'objets** : Séparation stricte entre les données statiques (icônes, descriptions) et les données dynamiques (quantité, durabilité). Cela réduit la charge mémoire et facilite l'équilibrage du jeu.
3. **Blueprint Interfaces pour la communication** : Élimination des dépendances directes (*Casting*) entre le personnage et les objets du monde, garantissant un projet évolutif, propre et performant.

---

## 3. Déclaration d'Intention sur l'Usage de l'IA (AI Statement of Intent)

### 3.1 Vue d'ensemble et philosophie

L'intégralité du projet (programmation des systèmes, logique de jeu, architecture Blueprint) a été réalisée **manuellement** sous Unreal Engine 5. L'intelligence artificielle n'a en aucun cas été utilisée pour générer du code ou des graphes Blueprints automatiquement à la place du développeur.

L'IA a été sollicitée de manière ponctuelle comme **outil d'assistance, d'optimisation de flux de travail et de revue technique**, garantissant que chaque ligne de Blueprint, chaque structure et chaque décision d'architecture reste **parfaitement comprise, vérifiée, testée et expliquable**.

---

### 3.2 Outils Utilisés

- **Assistants LLM (Large Language Models)** : Utilisés pour la recherche de documentation, le rappel de nœuds Blueprints spécifiques et l'assistance sur la conception des Widgets UMG.
- **Outils de rédaction automatisée** : Utilisés pour la structuration et la mise en forme de la présente documentation technique en Markdown.

---

### 3.3 Domaines d'Application de l'IA

1. **Rappel et vérification de nœuds Blueprints spécifiques** : Identification de fonctions ou de nœuds rarement utilisés ou oubliés.
2. **Conception et intégration des Widgets (UMG)** : Assistance conceptuelle et méthodologique sur la liaison de données (*Data Binding*) et le flux de travail des interfaces utilisateur.
3. **Validation des choix d'architecture** : Vérification des conventions de nommage et confirmation des bonnes pratiques (ex: utilisation de Data Assets et d'Interfaces).
4. **Mise en forme de la documentation** : Rédaction, organisation et formatage propre du cahier des charges et de la documentation technique.

---

### 3.4 Raisons de l'Utilisation

- **Gain de temps** : Éviter de longues recherches dans les documentations officielles pour retrouver la nomenclature exacte de certains nœuds Engine ou fonctions UMG.
- **Dépassement de points de blocage** : L'intégration UI (Widgets) sous Unreal Engine présentant une courbe d'apprentissage spécifique, l'IA a servi de tutoriel interactif sur mesure.
- **Rigueur architecturale** : Valider au préalable les choix de structure pour s'assurer qu'ils répondent aux standards de développement Unreal Engine.

---

### 3.5 Modalités d'Utilisation

1. **Questionnement ciblé** : Formulation de requêtes précises sur des problématiques ponctuelles (ex: *"Comment lier un composant d'inventaire à une grille de Widget UMG sans couplage fort ?"*).
2. **Analyse critique** : Évaluation immédiate des réponses fournies au regard des contraintes du projet.
3. **Implémentation manuelle** : Création manuelle de tous les nœuds, variables, fonctions et liaisons directement dans l'éditeur d'Unreal Engine.
4. **Tests et validation** : Exécution de tests en jeu pour valider le comportement physique, la transmission des interfaces et la stabilité des données.

---

### 3.6 Bénéfices Obtenus

- **Fluidité de développement** : Résolution rapide des doutes sur l'UI et les Widgets.
- **Architecture propre** : Confirmation des choix techniques clés (`PrimaryDataAsset`, `BlueprintInterface`, `ActorComponent`).
- **Pédagogie et montée en compétences** : Compréhension approfondie des mécaniques UMG grâce aux explications fournies par l'IA lors des blocages.
- **Documentation professionnelle** : Génération d'une documentation claire, sobre, structurée et conforme aux attentes.

---

### 3.7 Limites Rencontrées

- **Incapacité d'implémentation directe** : L'IA ne pouvant pas manipuler l'éditeur visuel de Blueprints, toute suggestion a dû être interprétée et reconstruite manuellement.
- **Hallucinations sur certains nœuds** : Certaines suggestions de fonctions ou de nœuds n'existaient pas sous la forme exacte décrite ou étaient obsolètes par rapport aux versions récentes d'UE5.
- **Nécessité de contrôle systématique** : Aucune recommandation d'architecture n'a pu être appliquée sans vérification préalable de sa faisabilité au sein de la structure du projet.

---

### 3.8 Conclusion et Responsabilité

Tout le code visuel, la logique Blueprint et les composants intégrés au projet sont entièrement maîtrisés et validés par le développeur. L'IA a servi de partenaire de réflexion et d'assistant documentaire, garantissant la qualité finale sans se substituer à la conception technique humaine.
