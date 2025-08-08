### Editor Refactor Plan

This plan targets making the editor more extensible, testable, and maintainable by decomposing the monolithic `EditorSubsystem` update loop, centralizing asset/renderer lifecycle handling in the engine layer, and introducing clear extension points for panels, commands, and tooling.

---

### Goals

- Reduce responsibilities of `EditorSubsystem::OnUpdate` to orchestration only.
- Eliminate manual renderer resource acquisition from editor code after importing assets.
- Provide a consistent, discoverable extension model for panels, menus, commands, and services.
- Improve stability (ImGui window lifecycle), logging, and background task handling.
- Keep runtime/editor boundaries clean and explicit.

---

### Pain Points Observed

- Monolithic `EditorSubsystem::OnUpdate` with mixed responsibilities (menu, docking, panels, utils, drag-n-drop, gizmos, camera, scene update).
- Manual calls to `ISceneRenderer::AcquireResourceIndex` from editor code after model import (see TODO) — this should be engine responsibility.
- Duplicate/unused `PanelManager` concept with incorrect initialization and not integrated with `EditorSubsystem`.
- Selection, viewport focus, and runtime flags are scattered and not shared across panels.
- ImGui stability issue: crash when closing/hiding windows that contain tables.
- Logging panel uses an unbounded vector with TODO for ring-buffer.
- Hard-coded project path and file names inside editor layer.
- No formal command/action system (shortcuts, undo/redo, command palette) — logic spread across UI.

---

### Target Architecture

- EditorCore
  - **EditorLayer (formerly `EditorSubsystem`)**: orchestrates high-level flow; delegates to services/panels.
  - **EditorContext**: shared state container (current scene, selection, runtime flag, viewport focus/hover, debug toggles, project path). Accessible to panels/services.
  - **Services** (singletons or owned by `EditorLayer`):
    - `PanelManager`: panel registration, lifecycle, per-panel context wiring.
    - `CommandRegistry` + `UndoRedoHistory`: actions with shortcuts and undo/redo.
    - `SelectionService`: single source of truth for selected/hovered entity, emits events.
    - `GizmoController`: transform tools and snapping modes; draws gizmos.
    - `ProjectService`: open/save/new project, working directory management.
    - `AssetOperations`: drag-and-drop handling, import queues, progress reporting.
    - `BackgroundTaskService`: job submission + progress UI integration.
    - `ShortcutService`: centralizes key bindings.

- Panel System
  - Standard `EditorPanel` interface extended with lifecycle hooks: `OnAttach`, `OnDetach`, `OnUpdate`, `OnEvent`, `OnMenu` (optional), and `Open(bool)`.
  - Panels register via `PanelManager::AddPanel(key, panel)` and can contribute menu items via `OnMenu` or a `MenuModel` provider.
  - Example built-in panels: Scene Hierarchy, Properties, Content Browser, Logs, Viewport, Path Tracing Settings.
  - Panels get `EditorContext*` for shared state (instead of direct member wiring across the editor).

- Menu and Docking
  - Extract main menu bar, popups, and docking layout into a dedicated `EditorFrame` module.
  - Menu items are built from `CommandRegistry` (data-driven; supports enable/disable, checked state, shortcuts).

- Command/Action System
  - Define `ICommand { Execute(); Undo(); }` with a history stack to enable undo/redo for scene edits, component changes, and asset operations.
  - Shortcuts map to commands; menu items bind to commands; panels fire commands rather than performing logic inline.

- Selection and Gizmos
  - `SelectionService` exposes `SetSelected(Entity)`, `GetSelected()`, `OnSelectionChanged`.
  - `GizmoController` reads selection from `SelectionService` and handles ImGuizmo interactions, snapping, and local/world modes.

- Asset Pipeline Integration (Engine-level)
  - Move renderer resource acquisition out of editor:
    - Use entt construction/destruction signals in `Scene` to react to `MeshComponent` additions/removals.
    - On `on_construct<MeshComponent>`: acquire renderer indices for mesh and material.
    - On `on_destroy<MeshComponent>`: release corresponding renderer indices.
  - Ensure `ISceneRenderer` has symmetric `ReleaseResourceIndex` for meshes/materials in addition to images.
  - `DeviceMaterialPool` and material texture sampling already acquire texture indices; keep that responsibility in engine.
  - Model import flow produces scene entities (or a prefab graph). Engine hooks ensure renderer is ready without editor intervention.

- Asset Import and Drag-and-Drop
  - Create `AssetOperations` with:
    - `ImportModel(path) -> Entity or PrefabHandle` (async, reports progress; creates entities in scene).
    - `InstantiatePrefab(handle) -> Entity`.
  - Content Browser drag-and-drop targets call into `AssetOperations` only; no direct renderer calls.

- Project/Scene Management
  - Extract project open/save/new into `ProjectService` (manages working directory, recent projects, autosave, layout persistence).
  - Remove hard-coded project path and filename from editor layer.

- Input and Focus
  - `ShortcutService` centralizes accelerators.
  - Viewport focus/hover handled by `ViewportPanel`, exposed in `EditorContext` for camera controls and gizmo locking.

- Logging and Diagnostics
  - Replace vector-backed logs with a bounded lock-free ring buffer and severity filtering. Provide a dedicated spdlog sink feeding the buffer.
  - Add a central diagnostic overlay panel (frame time, renderer stats, warnings).

- Stability with ImGui
  - Enforce the pattern: only draw panel contents when `Open` and `ImGui::Begin` return true; always match `BeginTable` with `EndTable`.
  - Tables and tree nodes must not hold references to temporary buffers beyond their frame; store persistent data in panel state.

---

### Concrete Refactors (Ordered)

1) Decompose `EditorSubsystem::OnUpdate`
   - Extract: `DrawMainMenuBar()`, `DrawDockspace()`, `UpdatePanels()`, `DrawDebugOverlay()`, `DrawUtilsWindow()`, `HandleDragAndDrop()`, `UpdateViewport()`.
   - Move gizmo logic into `GizmoController::UpdateAndDraw()`.
   - Replace direct selection handling with `SelectionService`.

2) Introduce `EditorContext`
   - Holds: `Scene* currentScene`, `bool inRuntime`, `bool viewportFocused`, `Entity selected`, flags (visualize colliders, cluster debug view), project path/filename.
   - Provide accessors + change events; pass pointer to all panels and services.

3) Adopt `PanelManager` fully (fix bugs and wire it in)
   - Fix incorrect registration (e.g., `scene_hierarchy` should create `SceneHierarchyPanel`, not `PropertiesPanel`).
   - Remove direct `m_HierarchyPanel`, `m_PropertiesPanel`, etc., from `EditorSubsystem`; use `PanelManager` for lifecycle and updates.
   - Allow panels to contribute to “View” menu by exposing their `DisplayName` and visibility state.

4) Engine-side resource acquisition via entt hooks
   - In `Scene` ctor or initialization, connect entt signals:
     ```cpp
     m_Registry.on_construct<MeshComponent>().connect<&Scene::OnMeshAdded>(this);
     m_Registry.on_destroy<MeshComponent>().connect<&Scene::OnMeshRemoved>(this);
     ```
   - Implement:
     ```cpp
     void Scene::OnMeshAdded(entt::registry& r, entt::entity e);
     void Scene::OnMeshRemoved(entt::registry& r, entt::entity e);
     ```
   - In `OnMeshAdded`, acquire indices for mesh and material via `m_Renderer->AcquireResourceIndex(...)`.
   - In `OnMeshRemoved`, release indices (add `ReleaseResourceIndex` overloads if missing).
   - Remove editor-side `AcquireResourceIndex` calls (e.g., after model import in the viewport drop target).

5) `ISceneRenderer` API symmetry
   - Add `ReleaseResourceIndex(Ref<Mesh>)` and `ReleaseResourceIndex(Ref<Material>)`.
   - Ensure thread safety mirrors `AcquireResourceIndex` locks.
   - Validate existing image release path; align naming.

6) Asset import pipeline
   - Add `AssetOperations::ImportModel(path)` to run importer and return either a root `Entity` or a `Prefab`.
   - Let the engine allocate renderer resources through the entt hooks when components are added to the scene.
   - Make import async; show progress and allow cancellation; after completion, auto-select the root entity via `SelectionService`.

7) Command system + undo/redo
   - Introduce `ICommand` and `CommandRegistry`.
   - Wrap entity/component mutations in commands (e.g., transform, add/remove component, delete entity, material assignment, import model).
   - Add `Ctrl+Z`/`Ctrl+Y` bindings; show a simple command history panel.

8) Viewport and Gizmos
   - Create `ViewportPanel` that: draws the scene image, owns camera controls when focused, handles drag-and-drop, and calls `GizmoController`.
   - Move ImGuizmo code out of editor core and bind to `SelectionService`.

9) Logging improvements
   - Implement a bounded ring buffer and a spdlog sink; support severity filters, search, and copy.
   - Optionally persist log to file per session.

10) ImGui stability fixes
   - Apply `if (isOpen && ImGui::Begin(...)) { ... } ImGui::End();` in all panels.
   - Ensure all `BeginTable` have matching `EndTable` and are only used within a successful `Begin` block.

11) Project settings and layout persistence
   - Move project paths into `ProjectService`, persist editor layout and per-panel open state.
   - Provide a “Recent Projects” menu fed by a small history file.

---

### Acceptance Criteria

- `EditorSubsystem::OnUpdate` is reduced to high-level orchestration; panel updates and gizmos are delegated.
- No editor code calls `ISceneRenderer::AcquireResourceIndex` for content assets; model import works without manual acquisition.
- Panels are registered via `PanelManager` and can be toggled from the “View” menu dynamically.
- Selection changes propagate across panels via `SelectionService`.
- Closing/hiding any panel that uses ImGui tables does not crash the editor.
- Logs panel uses a bounded buffer with filtering and no unbounded memory growth.
- Project open/save/new are handled by `ProjectService`; hard-coded paths removed.

---

### Suggested File/Module Changes

- `Editor/Source/EditorSubsystem.cpp`
  - Extract methods: `DrawMainMenuBar`, `DrawDockspace`, `HandleDragAndDrop`, `DrawDebug`, `UpdateViewport`.
  - Remove direct panel fields; use `PanelManager`.
  - Remove direct resource acquisition calls.

- `Editor/Source/PanelManager.*`
  - Fix default registrations and wire to `EditorContext`.
  - Add API for menu contributions and toggling visibility.

- `Editor/Source/EditorPanels/*`
  - Conform to lifecycle; use `Open(bool)` gating; no persistent references to transient UI buffers.
  - Add `ViewportPanel` and `GizmoController` integration.

- `Omniforce/Source/Scene/Scene.*`
  - Add entt `on_construct`/`on_destroy` hooks for `MeshComponent`.
  - Implement `OnMeshAdded/Removed` and call renderer acquire/release.

- `Omniforce/Source/Rendering/ISceneRenderer.*`
  - Add `ReleaseResourceIndex(Ref<Mesh>)` and `ReleaseResourceIndex(Ref<Material>)`.
  - Ensure thread-safety and mirror of acquire paths.

- `Editor/Source/Services/*` (new)
  - `EditorContext`, `SelectionService`, `CommandRegistry`, `UndoRedoHistory`, `ProjectService`, `AssetOperations`, `BackgroundTaskService`, `ShortcutService`.

---

### Migration & Phasing

- Phase 0: Prep
  - Fix `PanelManager` registrations and integrate with `EditorSubsystem` without functional changes.
  - Introduce `EditorContext`; pass to existing panels.

- Phase 1: Extract UI responsibilities
  - Move menu/dockspace/viewport/gizmos to dedicated modules.
  - Ensure selection is driven by `SelectionService`.

- Phase 2: Engine hooks for renderer resources
  - Implement entt hooks for `MeshComponent` and add missing release API.
  - Remove editor-side acquisition calls; verify import path renders correctly.

- Phase 3: Asset operations & async import
  - Add `AssetOperations` and progress UI; move DnD code to call it.

- Phase 4: Commands and undo/redo
  - Introduce `ICommand`, register common scene operations, wire shortcuts, add history panel.

- Phase 5: Logging and stability polish
  - Ring buffer logging, crash fixes in panels using tables, persist editor layout and recent projects.

---

### Notes on Implementation

- Entt hooks example:
  ```cpp
  // Scene initialization
  m_Registry.on_construct<MeshComponent>().connect<&Scene::OnMeshAdded>(this);
  m_Registry.on_destroy<MeshComponent>().connect<&Scene::OnMeshRemoved>(this);

  void Scene::OnMeshAdded(entt::registry& r, entt::entity e) {
      auto& comp = r.get<MeshComponent>(e);
      auto* assets = AssetManager::Get();
      m_Renderer->AcquireResourceIndex(assets->GetAsset<Mesh>(comp.mesh_handle));
      m_Renderer->AcquireResourceIndex(assets->GetAsset<Material>(comp.material_handle));
  }

  void Scene::OnMeshRemoved(entt::registry& r, entt::entity e) {
      auto& comp = r.get<MeshComponent>(e);
      auto* assets = AssetManager::Get();
      m_Renderer->ReleaseResourceIndex(assets->GetAsset<Mesh>(comp.mesh_handle));
      m_Renderer->ReleaseResourceIndex(assets->GetAsset<Material>(comp.material_handle));
  }
  ```

- ImGui panel pattern:
  ```cpp
  if (m_IsOpen && ImGui::Begin("Properties", &m_IsOpen)) {
      if (ImGui::BeginTable("props", 2, flags)) {
          // ...
          ImGui::EndTable();
      }
      ImGui::End();
  }
  ```

---

### Risks & Mitigations

- Renderer resource lifetime mismatches — add symmetric release paths and audit all `AcquireResourceIndex` call sites.
- Potential performance impact from entt hooks — batch acquire/release or debounce if needed.
- Behavior changes in panel update order — order panels in `PanelManager` deterministically and allow explicit priorities.
- Undo/redo complexity — begin with core operations (transform/entity add/remove/component add/remove), expand iteratively.

---

### Outcome

After these changes, the editor becomes a thin orchestrator over a set of well-defined services and panels. Asset/renderer coupling moves into the engine lifecycle, and adding new panels or features becomes a matter of registering a panel or command, rather than editing a monolithic update loop.


