document.addEventListener('DOMContentLoaded', function() {
    // Update time function for the current-time element
    function updateTime() {
        const now = new Date();
        const timeElement = document.getElementById('current-time');
        if (timeElement) {
            timeElement.textContent = now.toLocaleTimeString();
        }
    }
    
    // Initial call and set interval
    updateTime();
    setInterval(updateTime, 1000);
    
    // Add click event to h1
    const heading = document.querySelector('h1');
    if (heading) {
        heading.addEventListener('click', function() {
            alert('WebServ is working correctly with JavaScript!');
        });
    }
    
    // Add click effect
    document.querySelector('h1').addEventListener('click', function() {
        this.style.color = '#' + Math.floor(Math.random()*16777215).toString(16);
    });
});