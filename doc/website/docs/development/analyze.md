---
title: MNE Analyze
sidebar_label: MNE Analyze
---

# MNE Analyze Development

MNE Analyze is the offline data analysis and visualization application in MNE-CPP. It uses a plugin architecture where all functionality — data loading, filtering, visualization, source localization — is implemented as plugins that communicate via an event system.

The guides in this section cover how to extend MNE Analyze:

- **[Creating a Plugin](analyze-plugin.md)** — Detailed guide to implementing a new MNE Analyze plugin, including GUI elements, menus, and controls.
- **[Event System](analyze-event.md)** — How plugins communicate with each other using the built-in event manager.
- **[Data Model](analyze-datamodel.md)** — How to create new data models for loading and displaying custom data types.
