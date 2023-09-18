var WebSocketServer = require('websocket').server;
var http = require('http');
var os = require('os'); 
var fs = require('fs'); 

var server = http.createServer(function (request, response) {
    console.log((new Date()) + ' Received request for ' + request.url);
    if (request.url === '/') {
        fs.readFile('index.html', 'utf8', function (err, data) {
            if (err) {
                response.writeHead(500);
                response.end('Error loading index.html');
            } else {
                response.writeHead(200, { 'Content-Type': 'text/html' });
                response.end(data);
            }
        });
    } else {
        response.writeHead(404);
        response.end();
    }
});

server.listen(5000, '0.0.0.0', function () {
    var ifaces = os.networkInterfaces();
    var localIP = '';
    for (var dev in ifaces) {
        ifaces[dev].forEach(function (details) {
            if (details.family === 'IPv4' && !details.internal) {
                localIP = details.address;
            }
        });
    }

    console.log((new Date()) + ' Server is listening on port 5000, local IP: ' + localIP);
});

wsServer = new WebSocketServer({
    httpServer: server,
    autoAcceptConnections: false
});

function originIsAllowed(origin) {
    return true;
}
function removePrefix(inputString) {
    return inputString.replace(/^(web: |temperature: )/, '');
}

let connections = {};
wsServer.on('request', function (request) {
    console.log(request);
    if (!originIsAllowed(request.origin)) {
        request.reject();
        console.log((new Date()) + ' Connection from origin ' + request.origin + ' rejected.');
        return;
    }

    var connection = request.accept(null, request.origin);
    
    console.log((new Date()) + ' Connection accepted.');

    connection.on('message', function (message) {
        if (message.type === 'utf8') {
            
            if (message.utf8Data.startsWith("web: ")) {
                //connection.sendUTF("Hello to web");
                if (removePrefix(message.utf8Data) == "initiate") {
                    connections["web"] = connection;
                } else {
                    console.log('sending:')
                    console.log(removePrefix(message.utf8Data))
                    connections["temperature"].sendUTF(removePrefix(message.utf8Data));
                }
            }
            else {
                //connection.sendUTF("Hello to harware");
                if (removePrefix(message.utf8Data) == "initiate") {
                    connections["temperature"] = connection;

                }
                else {
                    try {
                        connections["web"].sendUTF(message.utf8Data);
                    } catch (e) {

                    }

                    console.log('Received Message: ' + removePrefix(message.utf8Data));
                }


            }

        }
        
    });

    connection.on('close', function (reasonCode, description) {
        console.log((new Date()) + ' Peer ' + connection.remoteAddress + ' disconnected.');
    });
});
