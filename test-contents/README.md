# Test Contents

This directory holds reference captures of real web pages used to validate the browser's layout engine.

## Folder Structure

Each web page gets its own folder named descriptively:

```
test-contents/
  google-homepage/
    screenshot.png          # Full-page screenshot from Chrome
    source.html             # Page source saved as HTML
    layout-geometry.json    # Flex/grid layout geometry exported from Chrome DevTools
  github-repo-page/
    screenshot.png
    source.html
    layout-geometry.json
  ...
```

## File Descriptions

| File                  | Description                                                                 |
|-----------------------|-----------------------------------------------------------------------------|
| `screenshot.png`      | A full-page screenshot (PNG or JPG) taken in Chrome at a known viewport size |
| `source.html`         | The page source code saved via "Save As" or DevTools                        |
| `layout-geometry.json`| Flex/grid layout geometry extracted from Chrome DevTools (see below)         |

## How to Capture Layout Geometry from Chrome DevTools

1. Open the page in Chrome and press **F12** to open DevTools.
2. Select the element of interest in the **Elements** panel.
3. Look at the **Layout** sidebar pane -- it shows flex/grid overlays.
4. To export computed geometry programmatically, paste this snippet into the **Console**:

```js
// Run on a flex or grid container to extract child geometry
function extractLayoutGeometry(selector) {
  const container = document.querySelector(selector);
  if (!container) return null;

  const style = getComputedStyle(container);
  const children = Array.from(container.children).map((child, i) => {
    const rect = child.getBoundingClientRect();
    const cs = getComputedStyle(child);
    return {
      index: i,
      tagName: child.tagName,
      className: child.className,
      rect: { x: rect.x, y: rect.y, width: rect.width, height: rect.height },
      flexGrow: cs.flexGrow,
      flexShrink: cs.flexShrink,
      flexBasis: cs.flexBasis,
      gridArea: cs.gridArea,
      gridColumn: cs.gridColumn,
      gridRow: cs.gridRow
    };
  });

  return {
    selector,
    containerDisplay: style.display,
    flexDirection: style.flexDirection,
    flexWrap: style.flexWrap,
    justifyContent: style.justifyContent,
    alignItems: style.alignItems,
    gridTemplateColumns: style.gridTemplateColumns,
    gridTemplateRows: style.gridTemplateRows,
    gap: style.gap,
    containerRect: container.getBoundingClientRect(),
    viewport: { width: window.innerWidth, height: window.innerHeight },
    children
  };
}

// Usage: copy(extractLayoutGeometry('body > div.main'));
// Then paste the result into layout-geometry.json
```

5. Call `copy(extractLayoutGeometry('.your-selector'))` and paste the JSON into `layout-geometry.json`.
6. You can call it multiple times for different containers and store them as an array.

## Example layout-geometry.json

```json
[
  {
    "selector": "nav.main-nav",
    "containerDisplay": "flex",
    "flexDirection": "row",
    "justifyContent": "space-between",
    "alignItems": "center",
    "containerRect": { "x": 0, "y": 0, "width": 1280, "height": 64 },
    "viewport": { "width": 1280, "height": 720 },
    "children": [
      {
        "index": 0,
        "tagName": "DIV",
        "className": "logo",
        "rect": { "x": 16, "y": 12, "width": 120, "height": 40 },
        "flexGrow": "0",
        "flexShrink": "1",
        "flexBasis": "auto"
      }
    ]
  }
]
```

## Manifest

See `manifest.json` for a machine-readable index of all test pages.
