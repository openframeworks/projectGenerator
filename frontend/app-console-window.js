// detached console window - just appends every consoleMessage it's sent, same
// nl2br formatting as the docked console in app.js's consoleMessage()
const ipcRenderer = window.ipcWrapper;

ipcRenderer.on('consoleMessage', (event, msg) => {
	const text = (msg + '').replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br>\n$2');
	const p = document.createElement('p');
	p.innerHTML = text;
	document.getElementById('console').appendChild(p);
	window.scrollTo(0, document.body.scrollHeight);
});
