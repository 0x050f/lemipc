# lemipc

lemipc project's goal is to manipulate SysV ipcs. `man ipcs`
The project is about making a small game using shared memory and msgq to
communicate.

## Compilation
```
make
```

## Usage
```
usage: ./lemipc team_number
```

## Example
```
┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
│ │ │ │ │ │ │2│ │ │ │ │ │ │ │ │ │ │ │ │ │
├─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ │ │ │ │ │1│ │ │ │ │ │ │ │ │ │ │ │ │ │ │
├─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ │ │ │ │ │ │2│ │ │ │ │ │ │ │ │ │ │ │ │ │
├─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
└─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘

┌─────────────────────────────────────────────────┐
│2: Ready for next turn !                         │
│1: Ready for next turn !                         │
│2: attack (x: 5, y: 1)                           │
│1: attack (x: 6, y: 0)                           │
│Player from team 2 moved (x: 6, y: 2)            │
│2: Ready for next turn !                         │
│2: Ready for next turn !                         │
│1: I'm dead !                                    │
│You win !                                        │
└─────────────────────────────────────────────────┘
```