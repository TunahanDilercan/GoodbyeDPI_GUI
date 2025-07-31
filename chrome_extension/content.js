// Content script for additional DPI bypass
console.log('🔧 DPI Bypass content script loaded');

// Domain fragmenter for specific sites
class DPIBypass {
  constructor() {
    this.isActive = true;
    this.init();
  }
  
  init() {
    this.fragmentWebSocket();
    this.modifyRequests();
    this.injectCustomCSS();
    
    // Discord specific bypasses
    if (window.location.hostname.includes('discord')) {
      this.discordSpecificBypass();
    }
    
    // Twitter specific bypasses  
    if (window.location.hostname.includes('twitter') || window.location.hostname.includes('x.com')) {
      this.twitterSpecificBypass();
    }
  }
  
  fragmentWebSocket() {
    // Override WebSocket for Discord real-time communication
    const originalWebSocket = window.WebSocket;
    window.WebSocket = function(url, protocols) {
      console.log('🔌 WebSocket intercepted:', url);
      
      // Fragment WebSocket URL if needed
      if (url.includes('discord') || url.includes('gateway')) {
        // Add random parameters to bypass DPI
        const separator = url.includes('?') ? '&' : '?';
        url += `${separator}bypass=${Math.random().toString(36).substring(7)}`;
      }
      
      const ws = new originalWebSocket(url, protocols);
      
      // Add custom headers via WebSocket extensions if possible
      const originalSend = ws.send;
      ws.send = function(data) {
        // Fragment large messages
        if (typeof data === 'string' && data.length > 1000) {
          const chunks = this.chunkString(data, 500);
          chunks.forEach((chunk, index) => {
            setTimeout(() => originalSend.call(this, chunk), index * 10);
          });
        } else {
          originalSend.call(this, data);
        }
      }.bind(this);
      
      return ws;
    };
  }
  
  modifyRequests() {
    // Override fetch with additional bypass techniques
    const originalFetch = window.fetch;
    window.fetch = function(url, options = {}) {
      // Add bypass headers
      options.headers = {
        'User-Agent': navigator.userAgent.replace('Chrome', 'Chromium'),
        'Accept-Encoding': 'gzip, deflate, br',
        'Accept-Language': 'en-US,en;q=0.9',
        'Cache-Control': 'no-cache',
        'Pragma': 'no-cache',
        'Sec-Fetch-Dest': 'empty',
        'Sec-Fetch-Mode': 'cors',
        'Sec-Fetch-Site': 'same-origin',
        ...options.headers
      };
      
      // Fragment URL if it's a Discord API call
      if (typeof url === 'string' && url.includes('discord')) {
        url = this.fragmentUrl(url);
      }
      
      console.log('🌐 Fetch intercepted:', url);
      return originalFetch.call(this, url, options);
    }.bind(this);
  }
  
  fragmentUrl(url) {
    try {
      const urlObj = new URL(url);
      
      // Add random parameters
      urlObj.searchParams.set('_bypass', Math.random().toString(36).substring(7));
      urlObj.searchParams.set('_ts', Date.now().toString());
      
      return urlObj.toString();
    } catch (e) {
      return url;
    }
  }
  
  chunkString(str, size) {
    const chunks = [];
    for (let i = 0; i < str.length; i += size) {
      chunks.push(str.slice(i, i + size));
    }
    return chunks;
  }
  
  discordSpecificBypass() {
    console.log('🎮 Applying Discord-specific bypasses');
    
    // Override Discord's API base URL manipulation
    if (window.GLOBAL_ENV) {
      const originalAPI = window.GLOBAL_ENV.API_ENDPOINT;
      if (originalAPI) {
        // Fragment API endpoint
        window.GLOBAL_ENV.API_ENDPOINT = this.addDnsParameters(originalAPI);
        console.log('📡 Discord API endpoint modified');
      }
    }
    
    // Monitor for Discord's real-time gateway
    const observer = new MutationObserver((mutations) => {
      mutations.forEach((mutation) => {
        if (mutation.type === 'childList') {
          // Look for WebSocket connections in network requests
          this.interceptNetworkRequests();
        }
      });
    });
    
    observer.observe(document.body, { childList: true, subtree: true });
    
    // Override Discord's analytics to reduce fingerprinting
    if (window.DiscordSentry) {
      window.DiscordSentry.setUser = () => {};
      window.DiscordSentry.setTag = () => {};
      console.log('🔒 Discord analytics disabled');
    }
  }
  
  twitterSpecificBypass() {
    console.log('🐦 Applying Twitter-specific bypasses');
    
    // Override Twitter's API calls
    if (window.TwitterAPI) {
      const originalRequest = window.TwitterAPI.request;
      window.TwitterAPI.request = function(...args) {
        // Add random delays to avoid pattern detection
        setTimeout(() => {
          return originalRequest.apply(this, args);
        }, Math.random() * 100);
      };
    }
  }
  
  addDnsParameters(url) {
    try {
      const urlObj = new URL(url);
      
      // Add parameters that can help with DNS resolution
      urlObj.searchParams.set('dns', 'cloudflare');
      urlObj.searchParams.set('bypass', 'true');
      
      return urlObj.toString();
    } catch (e) {
      return url;
    }
  }
  
  interceptNetworkRequests() {
    // Monitor network requests for additional bypass opportunities
    if (window.performance && window.performance.getEntriesByType) {
      const entries = window.performance.getEntriesByType('resource');
      const recentEntries = entries.slice(-10); // Last 10 requests
      
      recentEntries.forEach(entry => {
        if (entry.name.includes('discord') || entry.name.includes('twitter')) {
          console.log('📊 Network request detected:', entry.name);
        }
      });
    }
  }
  
  injectCustomCSS() {
    // Inject CSS to improve performance and reduce tracking
    const style = document.createElement('style');
    style.textContent = `
      /* Reduce tracking pixels */
      img[src*="analytics"], img[src*="tracking"], img[src*="pixel"] {
        display: none !important;
      }
      
      /* Optimize for better performance */
      * {
        image-rendering: optimizeSpeed !important;
      }
      
      /* Hide potential DPI detection elements */
      [data-testid*="analytics"], [class*="tracking"], [id*="pixel"] {
        visibility: hidden !important;
      }
    `;
    
    document.head.appendChild(style);
    console.log('🎨 Custom CSS injected for DPI bypass');
  }
}

// Initialize DPI bypass when DOM is ready
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', () => {
    new DPIBypass();
  });
} else {
  new DPIBypass();
}

// Listen for messages from background script
chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (request.action === 'reinject') {
    console.log('🔄 Reinitializing DPI bypass');
    new DPIBypass();
    sendResponse({ success: true });
  }
});

// Periodic reinitialization for persistent bypass
setInterval(() => {
  console.log('🔄 Periodic DPI bypass check');
  
  // Check if bypass is still active
  if (window.fetch.toString().includes('bypass')) {
    console.log('✅ DPI bypass still active');
  } else {
    console.log('⚠️ Reinitializing DPI bypass');
    new DPIBypass();
  }
}, 30000); // Check every 30 seconds
