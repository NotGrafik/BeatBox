# 🎧 BeatBox Soundboard

![C++](https://img.shields.io/badge/language-C%2B%2B-blue.svg)
![Qt](https://img.shields.io/badge/framework-Qt%20Creator-green.svg)
![Status](https://img.shields.io/badge/status-en%20cours-yellow)

## 📝 Description

**BeatBox Soundboard** est une application développée en **C++ avec Qt** permettant de jouer des sons de manière interactive. Ce projet a pour but de fournir une interface utilisateur simple et efficace pour :

- Importer et gérer des sons locaux
- Diffuser plusieurs sons simultanément
- Synchroniser la lecture de sons
- Proposer une interface de soundboard dynamique et intuitive
- Se connecter à un serveur distant pour les fonctionnalités avancées

Ce projet est destiné à un usage personnel ou éducatif, et peut être étendu pour des besoins de streaming, de création musicale ou d'animation audio.

---

## 🚀 Initialisation du projet

### 📦 Prérequis

- **Qt Creator** installé (version recommandée : `>= 6.x`)
- **CMake** (si utilisé dans le projet)
- **Qt framework** avec les modules suivants :
  - QtMultimedia
  - QtWidgets
  - QtNetwork (si serveur utilisé)

### 🔧 Étapes d'installation

1. **Cloner le dépôt** :

```bash
git clone https://github.com/ton-utilisateur/beatbox-soundboard.git
cd beatbox-soundboard
```

2. **Ouvrir le projet dans Qt Creator** :
   
  - Fichier → Ouvrir un projet

  - Sélectionner le fichier CMakeLists.txt

3. **Configurer le build** :

  - Sélectionner un kit compatible (MinGW, MSVC, Clang, etc.)

  - Vérifier les modules Qt installés

4. **Compiler et exécuter** :

  - Cliquer sur "Run" (Ctrl+R) ou compiler avec Ctrl+B
