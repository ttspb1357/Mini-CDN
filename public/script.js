// Dynamically show which server is serving this page
document.addEventListener('DOMContentLoaded', function() {
    // Get the current path
    const currentPath = window.location.pathname;
    
    // Update the path display if element exists
    const pathElement = document.getElementById('path');
    if (pathElement) {
        pathElement.textContent = currentPath || '/';
    }
    
    // Make a request to get headers and show which server is serving
    // Note: In a real scenario with CORS, this would need adjustments
    const serverElement = document.getElementById('server');
    if (serverElement) {
        // Determine which server we're on based on port
        const hostname = window.location.hostname;
        const port = window.location.port;
        
        let serverName = 'Unknown';
        if (port === '8080' || port === '') {
            serverName = 'Origin Server (8080)';
        } else if (port === '8081') {
            serverName = 'Edge Asia (8081)';
        } else if (port === '8082') {
            serverName = 'Edge Europe (8082)';
        }
        
        serverElement.textContent = serverName;
    }
    
    console.log('Mini CDN Page Loaded');
    console.log('Current server: ' + (window.location.port || 8080));
});