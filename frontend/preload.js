const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('ipcWrapper', {
  send: (channel, data) => {
    ipcRenderer.send(channel, data);
  },
  sendSync: (channel, data) => {
    return ipcRenderer.sendSync(channel, data);
  },
  on: (channel, func) => {
    ipcRenderer.on(channel, (event, ...args) => func(...args));
  },
  path: {
    join: (...args) => ipcRenderer.sendSync('path', ['join', args]),
    isAbsolute: (p) => ipcRenderer.sendSync('path', ['isAbsolute', [p]]),
    relative: (from, to) => ipcRenderer.sendSync('path', ['relative', [from, to]]),
    resolve: (...args) => ipcRenderer.sendSync('path', ['resolve', args]),
    normalize: (...args) => ipcRenderer.sendSync('path', ['normalize', args]),
  },
  fs: {
    existsSync: (p) => ipcRenderer.sendSync('fs', ['existsSync', [p]]),
  },
  openExternal: (url) => ipcRenderer.sendSync('openExternal', url),
  showItemInFolder: (p) => ipcRenderer.sendSync('showItemInFolder', p),
});
