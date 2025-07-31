// DPI Bypass Extension - Background Service Worker
console.log('🚀 DPI Bypass Extension loaded');

// DNS-over-HTTPS servers
const DOH_SERVERS = {
  cloudflare: 'https://cloudflare-dns.com/dns-query',
  google: 'https://dns.google/dns-query',
  quad9: 'https://dns.quad9.net:5053/dns-query',
  opendns: 'https://doh.opendns.com/dns-query'
};

// Extension state
let isActive = true;
let currentDohServer = 'cloudflare';
let bypassedDomains = new Set([
  'discord.com',
  'discordapp.com', 
  'discord.gg',
  'twitter.com',
  'x.com',
  'youtube.com',
  'youtu.be',
  'googleapis.com'
]);

// Initialize extension
chrome.runtime.onInstalled.addListener(() => {
  console.log('DPI Bypass Extension installed');
  
  // Set default settings
  chrome.storage.sync.set({
    isActive: true,
    dohServer: 'cloudflare',
    bypassedDomains: Array.from(bypassedDomains)
  });
  
  // Set badge
  updateBadge();
});

// Load settings on startup
chrome.runtime.onStartup.addListener(loadSettings);
loadSettings();

function loadSettings() {
  chrome.storage.sync.get(['isActive', 'dohServer', 'bypassedDomains'], (result) => {
    isActive = result.isActive ?? true;
    currentDohServer = result.dohServer ?? 'cloudflare';
    bypassedDomains = new Set(result.bypassedDomains ?? Array.from(bypassedDomains));
    
    updateBadge();
    console.log('Settings loaded:', { isActive, currentDohServer, bypassedDomains: Array.from(bypassedDomains) });
  });
}

function updateBadge() {
  chrome.action.setBadgeText({
    text: isActive ? 'ON' : 'OFF'
  });
  
  chrome.action.setBadgeBackgroundColor({
    color: isActive ? '#4CAF50' : '#F44336'
  });
}

// DNS Resolution via DoH
async function resolveDnsOverHttps(domain, type = 'A') {
  if (!isActive) return null;
  
  try {
    const dohUrl = DOH_SERVERS[currentDohServer];
    const params = new URLSearchParams({
      name: domain,
      type: type,
      ct: 'application/dns-json'
    });
    
    const response = await fetch(`${dohUrl}?${params}`, {
      headers: {
        'Accept': 'application/dns-json'
      }
    });
    
    if (!response.ok) throw new Error(`DNS query failed: ${response.status}`);
    
    const data = await response.json();
    console.log(`🔍 DNS resolved for ${domain}:`, data);
    
    return data.Answer?.filter(record => record.type === (type === 'A' ? 1 : 28))
                     ?.map(record => record.data) || [];
                     
  } catch (error) {
    console.error(`❌ DNS resolution failed for ${domain}:`, error);
    return null;
  }
}

// Header modification for DPI bypass
chrome.webRequest.onBeforeSendHeaders.addListener(
  (details) => {
    if (!isActive) return {};
    
    const url = new URL(details.url);
    const domain = url.hostname.toLowerCase();
    
    // Check if domain should be bypassed
    const shouldBypass = Array.from(bypassedDomains).some(bypassDomain => 
      domain === bypassDomain || domain.endsWith('.' + bypassDomain)
    );
    
    if (!shouldBypass) return {};
    
    console.log(`🔄 Modifying headers for: ${domain}`);
    
    const headers = details.requestHeaders || [];
    
    // Add/modify headers for DPI bypass
    const modifications = [
      { name: 'User-Agent', value: getRandomUserAgent() },
      { name: 'Accept-Language', value: 'en-US,en;q=0.9,tr;q=0.8' },
      { name: 'Cache-Control', value: 'no-cache' },
      { name: 'X-Forwarded-For', value: getRandomIP() },
      { name: 'X-Real-IP', value: getRandomIP() }
    ];
    
    // Remove existing headers and add new ones
    const filteredHeaders = headers.filter(header => 
      !modifications.some(mod => mod.name.toLowerCase() === header.name.toLowerCase())
    );
    
    modifications.forEach(mod => {
      filteredHeaders.push(mod);
    });
    
    // Fragment Host header if it's Discord
    if (domain.includes('discord')) {
      const hostHeader = filteredHeaders.find(h => h.name.toLowerCase() === 'host');
      if (hostHeader) {
        // Split host header value to bypass DPI
        hostHeader.value = fragmentString(hostHeader.value);
      }
    }
    
    return { requestHeaders: filteredHeaders };
  },
  {
    urls: ["<all_urls>"]
  },
  ["requestHeaders"]
);

// Response header modification
chrome.webRequest.onHeadersReceived.addListener(
  (details) => {
    if (!isActive) return {};
    
    const url = new URL(details.url);
    const domain = url.hostname.toLowerCase();
    
    const shouldBypass = Array.from(bypassedDomains).some(bypassDomain => 
      domain === bypassDomain || domain.endsWith('.' + bypassDomain)
    );
    
    if (!shouldBypass) return {};
    
    const headers = details.responseHeaders || [];
    
    // Add CORS headers for better compatibility
    const corsHeaders = [
      { name: 'Access-Control-Allow-Origin', value: '*' },
      { name: 'Access-Control-Allow-Methods', value: 'GET, POST, PUT, DELETE, OPTIONS' },
      { name: 'Access-Control-Allow-Headers', value: 'Content-Type, Authorization, X-Requested-With' }
    ];
    
    corsHeaders.forEach(corsHeader => {
      const existingHeader = headers.find(h => h.name.toLowerCase() === corsHeader.name.toLowerCase());
      if (!existingHeader) {
        headers.push(corsHeader);
      }
    });
    
    return { responseHeaders: headers };
  },
  {
    urls: ["<all_urls>"]
  },
  ["responseHeaders"]
);

// Utility functions
function getRandomUserAgent() {
  const userAgents = [
    'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
    'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36',
    'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
    'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36'
  ];
  return userAgents[Math.floor(Math.random() * userAgents.length)];
}

function getRandomIP() {
  return `${Math.floor(Math.random() * 256)}.${Math.floor(Math.random() * 256)}.${Math.floor(Math.random() * 256)}.${Math.floor(Math.random() * 256)}`;
}

function fragmentString(str) {
  if (str.length <= 4) return str;
  const mid = Math.floor(str.length / 2);
  return str.substring(0, mid) + '\r\n\t' + str.substring(mid);
}

// Message handling from popup
chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  switch (request.action) {
    case 'toggle':
      isActive = !isActive;
      chrome.storage.sync.set({ isActive });
      updateBadge();
      sendResponse({ success: true, isActive });
      break;
      
    case 'setDohServer':
      currentDohServer = request.server;
      chrome.storage.sync.set({ dohServer: currentDohServer });
      sendResponse({ success: true });
      break;
      
    case 'addDomain':
      bypassedDomains.add(request.domain.toLowerCase());
      chrome.storage.sync.set({ bypassedDomains: Array.from(bypassedDomains) });
      sendResponse({ success: true });
      break;
      
    case 'removeDomain':
      bypassedDomains.delete(request.domain.toLowerCase());
      chrome.storage.sync.set({ bypassedDomains: Array.from(bypassedDomains) });
      sendResponse({ success: true });
      break;
      
    case 'getStatus':
      sendResponse({
        isActive,
        dohServer: currentDohServer,
        bypassedDomains: Array.from(bypassedDomains)
      });
      break;
      
    case 'testDns':
      resolveDnsOverHttps(request.domain)
        .then(ips => sendResponse({ success: true, ips }))
        .catch(error => sendResponse({ success: false, error: error.message }));
      return true; // Keep message channel open for async response
  }
});

// Tab update listener
chrome.tabs.onUpdated.addListener((tabId, changeInfo, tab) => {
  if (changeInfo.status === 'complete' && tab.url) {
    const url = new URL(tab.url);
    const domain = url.hostname.toLowerCase();
    
    const shouldBypass = Array.from(bypassedDomains).some(bypassDomain => 
      domain === bypassDomain || domain.endsWith('.' + bypassDomain)
    );
    
    if (shouldBypass && isActive) {
      console.log(`✅ DPI bypass active for: ${domain}`);
      
      // Inject additional bypass code if needed
      chrome.scripting.executeScript({
        target: { tabId },
        func: injectBypassCode,
        args: [domain]
      }).catch(console.error);
    }
  }
});

// Code to inject into pages
function injectBypassCode(domain) {
  console.log(`🔧 DPI bypass injected for: ${domain}`);
  
  // Override fetch for additional bypass
  const originalFetch = window.fetch;
  window.fetch = function(...args) {
    const [url, options = {}] = args;
    
    // Add custom headers
    options.headers = {
      ...options.headers,
      'Cache-Control': 'no-cache',
      'Pragma': 'no-cache'
    };
    
    return originalFetch.call(this, url, options);
  };
  
  // Override XMLHttpRequest
  const originalXHR = window.XMLHttpRequest;
  window.XMLHttpRequest = function() {
    const xhr = new originalXHR();
    const originalOpen = xhr.open;
    
    xhr.open = function(method, url, ...args) {
      const result = originalOpen.apply(this, [method, url, ...args]);
      
      // Add custom headers
      this.setRequestHeader('Cache-Control', 'no-cache');
      this.setRequestHeader('Pragma', 'no-cache');
      
      return result;
    };
    
    return xhr;
  };
}
