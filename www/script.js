document.addEventListener('DOMContentLoaded', function() {
    // Create a dynamic element
    const timeDisplay = document.createElement('div');
    timeDisplay.className = 'time-display';
    timeDisplay.style.marginTop = '20px';
    timeDisplay.style.padding = '10px';
    timeDisplay.style.backgroundColor = '#e9f7fe';
    timeDisplay.style.borderRadius = '5px';
    
    // Update time function
    function updateTime() {
        const now = new Date();
        timeDisplay.innerHTML = '<strong>Current time:</strong> ' + now.toLocaleTimeString();
    }
    
    // Initial call and set interval
    updateTime();
    setInterval(updateTime, 1000);
    
    // Add to document
    document.querySelector('.container').appendChild(timeDisplay);
    
    // Add click effect
    document.querySelector('h1').addEventListener('click', function() {
        this.style.color = '#' + Math.floor(Math.random()*16777215).toString(16);
    });
});