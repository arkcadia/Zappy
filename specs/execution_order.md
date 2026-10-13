# Execution Order

## Architecture Overview

```
Server <==> GUI
Server <==> AI
```

---

## Responsibilities

### Server
- Read all data from GUI
- Read all data from AI
- Execute all data from AI
- Spawn resources on the map
- Send all data to AI
- Send all data to GUI

### GUI
- Request data from server
- Read data from server
- Draw map
- Handle user events

### AI
- Analyze information
- Send action to server
- Read data from server
