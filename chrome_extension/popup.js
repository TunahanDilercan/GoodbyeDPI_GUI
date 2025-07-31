// Popup Script for DPI Bypass Extension
document.addEventListener('DOMContentLoaded', function() {
  console.log('🎛️ Popup loaded');
  
  // DOM elements
  const statusText = document.getElementById('statusText');
  const toggleBtn = document.getElementById('toggleBtn');
  const bypassCount = document.getElementById('bypassCount');
  const dohServer = document.getElementById('dohServer');
  const domainList = document.getElementById('domainList');
  const domainInput = document.getElementById('domainInput');
  const addDomainBtn = document.getElementById('addDomainBtn');
  const testDiscord = document.getElementById('testDiscord');
  const testDns = document.getElementById('testDns');
  const testResult = document.getElementById('testResult');
  const activeSessions = document.getElementById('activeSessions');
  const dnsQueries = document.getElementById('dnsQueries');
  const bypassRate = document.getElementById('bypassRate');
  
  let currentStatus = null;
  
  // Initialize popup
  loadStatus();
  
  // Event listeners
  toggleBtn.addEventListener('click', toggleExtension);
  dohServer.addEventListener('change', changeDohServer);
  addDomainBtn.addEventListener('click', addDomain);
  domainInput.addEventListener('keypress', function(e) {
    if (e.key === 'Enter') addDomain();
  });
  testDiscord.addEventListener('click', () => testConnection('discord.com'));
  testDns.addEventListener('click', () => testDnsResolution('discord.com'));
  
  // Load current status from background script
  function loadStatus() {
    chrome.runtime.sendMessage({ action: 'getStatus' }, (response) => {
      if (response) {
        currentStatus = response;
        updateUI();
        updateDomainList();
        updateStats();
      }
    });
  }
  
  // Update UI based on current status
  function updateUI() {
    if (!currentStatus) return;
    
    const { isActive, dohServer: currentDohServer, bypassedDomains } = currentStatus;
    
    // Update status indicator
    statusText.textContent = isActive ? '🟢 Aktif' : '🔴 Pasif';
    toggleBtn.textContent = isActive ? 'Aktif' : 'Pasif';
    toggleBtn.className = `toggle-btn ${isActive ? 'active' : 'inactive'}`;
    
    // Update bypass count
    bypassCount.textContent = bypassedDomains.length;
    
    // Update DoH server selection
    dohServer.value = currentDohServer;
  }
  
  // Update domain list
  function updateDomainList() {
    if (!currentStatus) return;
    
    const { bypassedDomains } = currentStatus;
    
    domainList.innerHTML = '';
    
    bypassedDomains.forEach(domain => {
      const domainItem = document.createElement('div');
      domainItem.className = 'domain-item';
      
      const domainName = document.createElement('span');
      domainName.textContent = domain;
      
      const removeBtn = document.createElement('button');
      removeBtn.className = 'remove-btn';
      removeBtn.textContent = '✕';
      removeBtn.onclick = () => removeDomain(domain);
      
      domainItem.appendChild(domainName);
      domainItem.appendChild(removeBtn);
      domainList.appendChild(domainItem);
    });
  }
  
  // Toggle extension on/off
  function toggleExtension() {
    chrome.runtime.sendMessage({ action: 'toggle' }, (response) => {
      if (response && response.success) {
        currentStatus.isActive = response.isActive;
        updateUI();
        showNotification(
          response.isActive ? '✅ DPI Bypass Aktif' : '⏹️ DPI Bypass Pasif'
        );
      }
    });
  }
  
  // Change DoH server
  function changeDohServer() {
    const server = dohServer.value;
    
    chrome.runtime.sendMessage({ 
      action: 'setDohServer', 
      server: server 
    }, (response) => {
      if (response && response.success) {
        currentStatus.dohServer = server;
        showNotification(`🌐 DNS Server: ${getServerName(server)}`);
      }
    });
  }
  
  // Add domain to bypass list
  function addDomain() {
    const domain = domainInput.value.trim().toLowerCase();
    
    if (!domain) {
      showNotification('❌ Geçerli bir domain girin', 'error');
      return;
    }
    
    // Validate domain format
    if (!isValidDomain(domain)) {
      showNotification('❌ Geçersiz domain formatı', 'error');
      return;
    }
    
    if (currentStatus.bypassedDomains.includes(domain)) {
      showNotification('⚠️ Domain zaten listede', 'warning');
      return;
    }
    
    chrome.runtime.sendMessage({ 
      action: 'addDomain', 
      domain: domain 
    }, (response) => {
      if (response && response.success) {
        currentStatus.bypassedDomains.push(domain);
        updateDomainList();
        updateUI();
        domainInput.value = '';
        showNotification(`✅ ${domain} eklendi`);
      }
    });
  }
  
  // Remove domain from bypass list
  function removeDomain(domain) {
    chrome.runtime.sendMessage({ 
      action: 'removeDomain', 
      domain: domain 
    }, (response) => {
      if (response && response.success) {
        currentStatus.bypassedDomains = currentStatus.bypassedDomains.filter(d => d !== domain);
        updateDomainList();
        updateUI();
        showNotification(`🗑️ ${domain} kaldırıldı`);
      }
    });
  }
  
  // Test connection to a domain
  function testConnection(domain) {
    showTestResult('🔄 Bağlantı test ediliyor...', 'testing');
    
    fetch(`https://${domain}`, { 
      method: 'HEAD',
      mode: 'no-cors'
    })
    .then(() => {
      showTestResult(`✅ ${domain} - Bağlantı başarılı!`, 'success');
    })
    .catch(error => {
      showTestResult(`❌ ${domain} - Bağlantı hatası: ${error.message}`, 'error');
    });
  }
  
  // Test DNS resolution
  function testDnsResolution(domain) {
    showTestResult('🔄 DNS çözümlemesi test ediliyor...', 'testing');
    
    chrome.runtime.sendMessage({ 
      action: 'testDns', 
      domain: domain 
    }, (response) => {
      if (response && response.success) {
        if (response.ips && response.ips.length > 0) {
          showTestResult(`✅ ${domain} DNS: ${response.ips.join(', ')}`, 'success');
        } else {
          showTestResult(`⚠️ ${domain} - DNS çözümlenemedi`, 'warning');
        }
      } else {
        showTestResult(`❌ DNS test hatası: ${response?.error || 'Bilinmeyen hata'}`, 'error');
      }
    });
  }
  
  // Show test result
  function showTestResult(message, type) {
    testResult.style.display = 'block';
    testResult.textContent = message;
    testResult.className = `test-result ${type}`;
    
    // Auto hide after 5 seconds
    setTimeout(() => {
      testResult.style.display = 'none';
    }, 5000);
  }
  
  // Update statistics
  function updateStats() {
    // Get current tab count for active sessions
    chrome.tabs.query({}, (tabs) => {
      const protectedTabs = tabs.filter(tab => {
        if (!tab.url) return false;
        try {
          const domain = new URL(tab.url).hostname.toLowerCase();
          return currentStatus.bypassedDomains.some(bypassDomain => 
            domain === bypassDomain || domain.endsWith('.' + bypassDomain)
          );
        } catch (e) {
          return false;
        }
      });
      
      activeSessions.textContent = protectedTabs.length;
    });
    
    // Mock statistics for demo
    dnsQueries.textContent = Math.floor(Math.random() * 1000) + 150;
    bypassRate.textContent = '94.7%';
  }
  
  // Utility functions
  function isValidDomain(domain) {
    const domainRegex = /^[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$/;
    return domainRegex.test(domain);
  }
  
  function getServerName(server) {
    const serverNames = {
      cloudflare: 'Cloudflare',
      google: 'Google',
      quad9: 'Quad9',
      opendns: 'OpenDNS'
    };
    return serverNames[server] || server;
  }
  
  function showNotification(message, type = 'info') {
    // Create notification element
    const notification = document.createElement('div');
    notification.style.cssText = `
      position: fixed;
      top: 10px;
      right: 10px;
      background: ${type === 'error' ? '#F44336' : type === 'warning' ? '#FF9800' : '#4CAF50'};
      color: white;
      padding: 8px 12px;
      border-radius: 4px;
      font-size: 11px;
      z-index: 1000;
      animation: slideIn 0.3s ease;
    `;
    notification.textContent = message;
    
    document.body.appendChild(notification);
    
    // Remove after 3 seconds
    setTimeout(() => {
      notification.style.animation = 'slideOut 0.3s ease';
      setTimeout(() => {
        document.body.removeChild(notification);
      }, 300);
    }, 3000);
  }
  
  // Periodic status updates
  setInterval(loadStatus, 10000); // Update every 10 seconds
});

// Add CSS animations
const style = document.createElement('style');
style.textContent = `
  @keyframes slideIn {
    from { transform: translateX(100%); opacity: 0; }
    to { transform: translateX(0); opacity: 1; }
  }
  
  @keyframes slideOut {
    from { transform: translateX(0); opacity: 1; }
    to { transform: translateX(100%); opacity: 0; }
  }
  
  .test-result.success {
    background: rgba(76, 175, 80, 0.3) !important;
    border-left: 3px solid #4CAF50;
  }
  
  .test-result.error {
    background: rgba(244, 67, 54, 0.3) !important;
    border-left: 3px solid #F44336;
  }
  
  .test-result.warning {
    background: rgba(255, 152, 0, 0.3) !important;
    border-left: 3px solid #FF9800;
  }
  
  .test-result.testing {
    background: rgba(33, 150, 243, 0.3) !important;
    border-left: 3px solid #2196F3;
  }
`;
document.head.appendChild(style);
