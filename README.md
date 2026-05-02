# Aim Offset Maker

AimOffsetMaker is an editor-only Unreal Engine plugin for fast Aim Offset authoring from a single animation sequence.

<img width="704" height="643" alt="image" src="https://github.com/user-attachments/assets/7f361e7f-2380-4806-8aed-3c54de38a98c" />
</br>

**It lets you:**
- pick a source `UAnimSequence`
- assign frames for each aim direction
- generate one-frame pose sequences
- optionally apply additive settings
- optionally compress generated animations
- optionally create and configure an Aim Offset asset automatically

Developed on Unreal Engine `5.7.4`.

Expected support:
- UE `5.7.4` confirmed
- likely compatible with most UE5 versions from `5.0+`

## Editor Only

This plugin is `Editor` only.

It does not run in packaged games.

Module type in `AimOffsetMaker.uplugin`:

```json
{
  "Name": "AimOffsetMaker",
  "Type": "Editor"
}
```

## Instruction

### Open from Animation Sequence Editor
<img width="1218" height="886" alt="Open from Animation Sequence Editor" src="https://github.com/user-attachments/assets/b6e21d3a-d85e-4956-942c-22601d4ac2b0" />

### Setup Frames
<img width="715" height="641" alt="Setup Frames" src="https://github.com/user-attachments/assets/b57c6ba3-2fa5-4eba-b5bd-65a020767862" />

#### Direction frame setup

Default directions: 

- CC = 0
- CU = 1
- UR = 2
- URB = 3
- UL = 4
- ULB = 5
- CR = 6
- CRB = 7
- CL = 8
- CLB = 9
- CD = 10
- DR = 11
- DRB = 12
- DL = 13
- DLB = 14

### Output Settings

Select folder and base name for assets.

<img width="699" height="408" alt="Output Settings" src="https://github.com/user-attachments/assets/e3849781-5d80-4e55-afce-f2a5a6618173" />

### Additive Settings

You can enable auto-additive settings.
If set:
```cpp
AdditiveAnimType = AAT_RotationOffsetMeshSpace;
RefPoseType = ABPT_AnimFrame;
RefFrameIndex = 0;
```

If a base animation is selected, it will be set as the reference.
If none is selected, the _CC asset will be used

<img width="688" height="369" alt="Additive Settings" src="https://github.com/user-attachments/assets/fb332350-242f-472a-bb58-ec3c68e8a6fb" />

### Aim Offset Settings

Optionally, you can create a pre-configured animation offset. It will use the same skeleton.
If `Setup Anim Offse` is set to true, it will automatically configure it based on a predefined template.

<img width="681" height="129" alt="Aim Offset Settings" src="https://github.com/user-attachments/assets/7aa678af-6cec-411b-8fdc-c8a6d7b17141" />


### Generated Anim Offset

After clicking 'Create' 
All the necessary assets and aim offsets will appear.

<img width="418" height="625" alt="Generated Assets" src="https://github.com/user-attachments/assets/64b124b2-be21-43f3-92d5-a80470bc6123" />

#### The configured animation offset looks like this

<img width="2015" height="1194" alt="Generated Anim Offset" src="https://github.com/user-attachments/assets/1c699ca0-3f8e-4045-b9e4-c7cfe3884660" />

## Meta Data Curve Action

<img width="426" height="194" alt="MetaData Curve" src="https://github.com/user-attachments/assets/9fcc74d1-38de-4d0e-898c-0bb56e050b4c" />

### Open from Content Browser

Right click one or more `UAnimSequence` assets in the Content Browser:

`Asset Actions -> Apply Meta Data Curve`

The action uses a curve icon in the context menu.

### Apply Meta Data Curve dialog

<img width="354" height="174" alt="Enter meta name" src="https://github.com/user-attachments/assets/1605e274-775d-4729-9c29-540bd755a3db" />

A small dialog will appear with:

- `Enter name`
- `Apply`
- `Cancel`

If cancelled, nothing is changed.

### What it does

For every selected animation sequence, the plugin:

- finds or creates a float metadata curve with the entered name
- forces the curve to be marked as `Metadata`
- sets the curve value to `1.0`
- marks the asset dirty if changed

If the curve already exists and is not `1.0`, it is updated.

### Progress and notifications

When applying metadata curves:

- a progress notification appears in the bottom-right corner
- a finish notification appears when the operation ends

The finish notification shows:

- curve name
- how many sequences were added
- how many sequences were updated
- how many sequences failed

### Multi-selection support

The metadata curve action supports applying the same curve to multiple selected `UAnimSequence` assets in one operation.

## Features

### Source animation selection

- opens from the Animation Sequence Editor
- defaults to the currently opened animation sequence
- can switch to another `UAnimSequence` before creation



Each direction gets its own frame index.

Validation checks:
- every direction has a frame
- every frame is unique
- every frame is inside the source animation range

### Output settings

- output folder picker
- base name for generated sequences
- optional animation compression

Generated sequence naming:

```text
BaseName_Direction
```

Example:

```text
AS_Crouch_AO_UR
AS_Crouch_AO_CL
AS_Crouch_AO_DLB
```

### Additive settings

Optional additive setup for all generated sequences:

- `Mesh Space` additive
- base pose type set to animation frame
- reference frame index set to `0`

Two additive modes:

1. Use selected animation
2. Use generated `CC` pose as base pose

Also includes:
- optional base pose animation picker
- preview viewport for the selected additive animation
- warning if additive mode needs a base animation and none is selected

### Aim Offset creation

Optional automatic Aim Offset asset creation:

- enabled by default
- custom asset name
- optional automatic setup

Default created asset name:

```text
<CurrentAnimName>_AnimOffset
```

If setup is enabled, the plugin configures:

Horizontal axis:
- Name: `Yaw`
- Min: `-180`
- Max: `180`
- Snap To Grid: `true`
- Smoothing Time: `0.1`

Vertical axis:
- Name: `Pitch`
- Min: `-90`
- Max: `90`
- Snap To Grid: `true`
- Smoothing Time: `0.1`

Other settings:
- Sample Weight Smoothing Speed: `2.0`

Samples are auto-filled from the generated direction poses.

### Quality of life

- integrated into Animation Sequence Editor only
- compact setup dialog
- collapsible categories
- tooltips across the UI
- progress notification during creation
- success/failure notification after completion
- created assets are synced in Content Browser
- Content Browser asset action for animation metadata curves
- curve icon in context menu for metadata curve action
- progress notification during metadata curve apply
- success/failure notification after metadata curve apply
- supports multi-select animation sequence processing

## How It Works

1. Open any animation sequence in Unreal Editor.
2. In the sequence editor, open `Window -> Asset Editor -> Anim Offset Maker`.
3. Confirm or change the source animation.
4. Set frame indices for all directions.
5. Choose output folder and base name.
6. Configure additive settings if needed.
7. Configure Aim Offset creation if needed.
8. Click `Create`.

The plugin then:
- samples the pose at each selected frame
- creates a new one-frame `UAnimSequence` per direction
- applies additive settings if enabled
- applies compression if enabled
- creates an Aim Offset asset if enabled
- configures and fills the Aim Offset samples if enabled

### Metadata curve workflow

1. Select one or more animation sequences in the Content Browser.
2. Open `Asset Actions -> Apply Meta Data Curve`.
3. Enter the metadata curve name.
4. Click `Apply`.

The plugin then:
- creates the metadata curve if missing
- updates it if it already exists
- sets its value to `1.0`
- shows progress and completion notifications

## Installation

### Project plugin

Copy the plugin into:

```text
<YourProject>/Plugins/AimOffsetMaker
```

Then:

1. regenerate project files if needed
2. build the project
3. open Unreal Editor
4. enable the plugin if it is not enabled automatically

``YourProject.uproject``:
```json
	"Plugins": [
		{
			"Name": "AimOffsetMaker",
			"Enabled": true
		}
	]
```

### Engine plugin

You can also place it in:

```text
<UE>/Engine/Plugins/AimOffsetMaker
```

## Usage Notes

- source asset must be a valid `UAnimSequence`
- source animation must have a valid skeleton
- source animation must have a usable preview mesh for pose extraction
- output folder must already exist in the project
- frame indices must be unique
- additive external animation is required only when `Use Animation` is enabled
- metadata curve apply works only on `UAnimSequence` assets
- multiple animation sequences can be processed at once
- entered metadata curve name must not be empty

## Validation

Before creation, the plugin validates:

- source sequence exists
- output folder is set
- output folder path is valid
- output folder exists in the project
- base name is set
- Aim Offset asset name is set when Aim Offset creation is enabled
- additive base animation is set when required
- every direction frame exists
- every direction frame is unique
- every direction frame is inside the source sequence frame range

For metadata curve apply, the plugin validates:

- at least one animation sequence is selected
- all selected assets are `UAnimSequence`
- curve name is not empty

## Limitations

- editor only
- designed around animation sequence workflow
- requires a valid preview mesh for reliable pose extraction
- currently focused on 2D Aim Offset style setup from predefined direction samples

## License

Licensed under [MIT](LICENSE)

