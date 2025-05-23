/**
 * Common JavaScript for all script language pages
 * Provides interactive elements and dynamic content loading
 */

document.addEventListener('DOMContentLoaded', function() {
    // Add syntax highlighting to code blocks (optional effect)
    highlightCodeBlocks();
    
    // Handle any collapsible sections
    setupCollapsibleSections();
    
    // Setup tabs if they exist
    setupTabs();
    
    // Add click handlers to run buttons if they exist
    setupRunButtons();
});

/**
 * Adds simple syntax highlighting to code blocks
 */
function highlightCodeBlocks() {
    const codeBlocks = document.querySelectorAll('.code-block');
    
    codeBlocks.forEach(function(block) {
        // Check if the block already has highlighting applied
        if (block.dataset.highlighted === 'true') return;
        
        let content = block.innerHTML;
        
        // Highlight comments
        content = content.replace(/(#.*)/g, '<span class="hl-comment">$1</span>');
        content = content.replace(/(\/\/.*)/g, '<span class="hl-comment">$1</span>');
        content = content.replace(/(\/\*[\s\S]*?\*\/)/g, '<span class="hl-comment">$1</span>');
        
        // Highlight strings
        content = content.replace(/(".*?")/g, '<span class="hl-string">$1</span>');
        content = content.replace(/('.*?')/g, '<span class="hl-string">$1</span>');
        
        // Highlight keywords (add more as needed)
        const keywords = [
            'function', 'def', 'class', 'if', 'else', 'elif', 'while', 'for', 
            'return', 'import', 'from', 'as', 'try', 'except', 'finally', 
            'require', 'use', 'my', 'sub', 'package', 'module', 'include'
        ];
        
        keywords.forEach(function(keyword) {
            const regex = new RegExp('\\b' + keyword + '\\b', 'g');
            content = content.replace(regex, '<span class="hl-keyword">' + keyword + '</span>');
        });
        
        block.innerHTML = content;
        block.dataset.highlighted = 'true';
    });
}

/**
 * Sets up collapsible sections
 */
function setupCollapsibleSections() {
    const collapsibles = document.querySelectorAll('.collapsible-header');
    
    collapsibles.forEach(function(header) {
        header.addEventListener('click', function() {
            const content = this.nextElementSibling;
            
            if (content.style.maxHeight) {
                content.style.maxHeight = null;
                this.classList.remove('active');
            } else {
                content.style.maxHeight = content.scrollHeight + "px";
                this.classList.add('active');
            }
        });
    });
}

/**
 * Sets up tab navigation if it exists on the page
 */
function setupTabs() {
    const tabGroups = document.querySelectorAll('.tab-container');
    
    tabGroups.forEach(function(tabGroup) {
        const tabs = tabGroup.querySelectorAll('.tab');
        const tabContents = tabGroup.querySelectorAll('.tab-content');
        
        tabs.forEach(function(tab, index) {
            tab.addEventListener('click', function() {
                // Deactivate all tabs and contents
                tabs.forEach(t => t.classList.remove('active'));
                tabContents.forEach(c => c.classList.remove('active'));
                
                // Activate the clicked tab and corresponding content
                tab.classList.add('active');
                tabContents[index].classList.add('active');
            });
        });
        
        // Activate the first tab by default
        if (tabs.length > 0 && tabContents.length > 0) {
            tabs[0].classList.add('active');
            tabContents[0].classList.add('active');
        }
    });
}

/**
 * Sets up script execution buttons
 */
function setupRunButtons() {
    const runButtons = document.querySelectorAll('.run-script-btn');
    
    runButtons.forEach(function(button) {
        button.addEventListener('click', function() {
            const scriptId = button.dataset.scriptId;
            const outputContainer = document.getElementById(scriptId + '-output');
            
            if (!outputContainer) return;
            
            // Show loading indicator
            outputContainer.innerHTML = '<div class="loading-spinner"></div><p>Running script...</p>';
            outputContainer.style.display = 'block';
            
            // Fetch the script execution result
            fetch(button.dataset.scriptUrl)
                .then(response => response.text())
                .then(data => {
                    outputContainer.innerHTML = '<pre>' + data + '</pre>';
                })
                .catch(error => {
                    outputContainer.innerHTML = '<p class="error">Error executing script: ' + error.message + '</p>';
                });
        });
    });
}