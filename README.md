# Minecraft Legacy - Project Index

This repository hosts the **projects.json** file that powers the [Minecraft Legacy](https://minecraftlegacy.com) website. The site reads this JSON directly from GitHub to display the community project list.

## Structure

```
projects.json    ← The single source of truth for all listed projects
```

### `projects.json` format

```json
{
  "repos": [
    {
      "name": "Eliather / LegacyCraft",
      "url": "https://github.com/Eliather/LegacyCraft",
      "priority": 17
    }
  ]
}
```

| Field      | Type     | Description                                                        |
| ---------- | -------- | ------------------------------------------------------------------ |
| `Eliather/LegacyCraft`     | `string` | Display name shown on the site (usually `owner / repo`)            |
| `https://github.com/Eliather/LegacyCraft`      | `string` | Full URL to the project (GitHub, Archive.org, etc.)                |
| `17` | `number` | Sort order - lower number = higher on the list. `1` is top priority |

## How to add a new project

### Option 1 - Open a Pull Request (recommended)

1. **Fork** this repo.
2. Edit `projects.json` - add your project entry to the `repos` array.
3. Pick a `priority` number. Use the next available number unless the maintainers say otherwise.
4. **Commit** with a clear message like `Add MyProject to project list`.
5. Open a **Pull Request** using the provided template.

### Option 2 - Request via Issue

If you don't want to edit JSON yourself, just open an **Issue** using the "Project Request" template and fill in the details. A maintainer will add it for you.

## Rules

- **Valid JSON only** - run your edit through a JSON validator before submitting.
- **No duplicate URLs** - check that your project isn't already listed.
- **Relevant projects only** - must be related to Minecraft Legacy / Console Edition.
- **One project per PR** - keeps reviews simple.

## How it works

The [Minecraft Legacy website](https://minecraftlegacy.com) fetches this file directly from:

```
https://raw.githubusercontent.com/MinecraftConsole/json/refs/heads/main/projects.json
```

Changes merged to `main` go live on the site immediately - no deploy needed.

## License

This project list is maintained by the Minecraft Legacy community. See [LICENSE](LICENSE) for details.
