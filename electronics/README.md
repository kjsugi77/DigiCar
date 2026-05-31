# electronics フォルダについて

DigiCar の電装・基板データをまとめたフォルダです。  
基板設計（KiCad）、製造用データ（Gerber）、回路図（PDF）を用途別に整理しています。

## フォルダ構成

electronics/
├── kicad/               ← KiCad プロジェクト一式（.kicad_pcb / .sch など）
├── gerber/              ← 基板製造用データ（Gerber ファイル）
└── schematic/           ← 回路図（PDF など）

## 使い方

- **kicad/**  
  基板の編集・修正を行う場合に使用します。  
  KiCad でプロジェクトを開くと、回路図と PCB を編集できます。

- **gerber/**  
  基板製造業者にそのまま提出できるデータです。  
  製造依頼時はこのフォルダ内の ZIP を使用します。

- **schematic/**  
  回路図を PDF で確認したい場合に使用します。  
  アプリ開発者や他のメンバーが電装仕様を理解する際にも便利です。


# About the electronics folder

This folder contains all electronic and PCB-related data for the DigiCar project.  
The contents are organized by purpose: PCB design (KiCad), manufacturing data (Gerber), and circuit diagrams (PDF).

## Folder Structure

electronics/
├── kicad/               ← KiCad project files (.kicad_pcb / .sch, etc.)
├── gerber/              ← Manufacturing data for PCB fabrication (Gerber files)
└── schematic/           ← Circuit diagrams (PDF and related files)

## Usage

- **kicad/**  
  Use this folder when editing or modifying the PCB design.  
  Open the project in KiCad to edit both the schematic and PCB layout.

- **gerber/**  
  Contains the data required by PCB manufacturers.  
  When ordering a PCB, submit the ZIP file from this folder.

- **schematic/**  
  Use this folder to view the circuit diagrams in PDF format.  
  Helpful for app developers or team members who need to understand the electrical design.
