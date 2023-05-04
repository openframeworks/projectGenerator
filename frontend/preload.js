const { contextBridge, ipcRenderer } = require('electron');

// TODO: need fix? (is this insecure?)
contextBridge.exposeInMainWorld('ipc_wrapper', {
    send: (event, data) => ipcRenderer.send(event, data),
    sendSync: (event, data) => ipcRenderer.sendSync(event, data),
    on: (channel, func) => ipcRenderer.on(channel, func),
    path: {
        join: (... args) => ipcRenderer.sendSync('path', ['join', args]),
        isAbsolute: (p) => ipcRenderer.sendSync('path', ['isAbsolute', [p]]),
        relative: (from, to) => ipcRenderer.sendSync('path', ['relative', [from, to]]),
        resolve: (... args) => ipcRenderer.sendSync('path', ['resolve', args]),
        normalize: (... args) => ipcRenderer.sendSync('path', ['normalize', args]),
    },
    fs: {
        existsSync: (p) => ipcRenderer.sendSync('fs', ['existsSync', [p]]),
    }
});
