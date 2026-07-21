/* ============================================================
   CWebServer — Interactive Demo Scripts
   ============================================================ */

(function () {
  'use strict';

  /* ---- Helpers ---- */
  function $(id) { return document.getElementById(id); }

  function showRawOutput(el, text, isError) {
    el.textContent = text;
    el.classList.add('show');
    el.classList.toggle('error', !!isError);
  }

  /* ==================================================================
     fetchAndDisplay — GET/POST via fetch, show raw + rendered table
     ================================================================== */
  function fetchAndDisplay(url, options, rawEl, tableEl) {
    rawEl.textContent = 'Loading...';
    rawEl.classList.add('show');
    rawEl.classList.remove('error');
    if (tableEl) { tableEl.classList.remove('show'); tableEl.innerHTML = ''; }

    fetch(url, options)
      .then(function (res) {
        return res.text().then(function (body) {
          var lines = [];
          lines.push('HTTP ' + res.status + ' ' + res.statusText);
          ['content-type', 'content-length'].forEach(function (h) {
            var v = res.headers.get(h);
            if (v) lines.push(h + ': ' + v);
          });
          lines.push('');
          var preview = body;
          if (preview.length > 2000) preview = preview.substring(0, 2000) + '\n... (truncated)';
          lines.push(preview);

          var isErr = res.status >= 400;
          showRawOutput(rawEl, lines.join('\n'), isErr);

          /* Try to parse and render HTML table */
          if (tableEl && !isErr) renderHtmlTable(body, tableEl);
        });
      })
      .catch(function (err) {
        showRawOutput(rawEl, 'Request failed:\n' + err.message, true);
      });
  }

  /* ==================================================================
     xhrRequest — XHR-based request (no browser auth dialog)
     ================================================================== */
  function xhrRequest(url, options, outEl) {
    outEl.textContent = 'Loading...';
    outEl.classList.add('show');
    outEl.classList.remove('error');

    var xhr = new XMLHttpRequest();
    xhr.open(options.method || 'GET', url, true);

    if (options.headers) {
      Object.keys(options.headers).forEach(function (k) {
        xhr.setRequestHeader(k, options.headers[k]);
      });
    }

    xhr.onload = function () {
      var lines = [];
      lines.push('HTTP ' + xhr.status + ' ' + xhr.statusText);
      var ct = xhr.getResponseHeader('Content-Type');
      if (ct) lines.push('content-type: ' + ct);
      var wwwa = xhr.getResponseHeader('WWW-Authenticate');
      if (wwwa) lines.push('www-authenticate: ' + wwwa);
      lines.push('');
      var body = xhr.responseText;
      if (body.length > 2000) body = body.substring(0, 2000) + '\n... (truncated)';
      lines.push(body);

      var isErr = xhr.status >= 400;
      showRawOutput(outEl, lines.join('\n'), isErr);
    };

    xhr.onerror = function () {
      showRawOutput(outEl, 'Request failed:\nNetwork error', true);
    };

    xhr.send(options.body || null);
  }

  /* ==================================================================
     renderHtmlTable — parse HTML, extract <table> or <body>, inject into div
     ================================================================== */
  function renderHtmlTable(html, container) {
    try {
      var parser = new DOMParser();
      var doc = parser.parseFromString(html, 'text/html');

      /* Prefer <table>, fall back to <body> content */
      var table = doc.querySelector('table');
      if (table) {
        table.removeAttribute('border');
        container.innerHTML = '';
        container.appendChild(table);
        container.classList.add('show');
        return;
      }

      var body = doc.querySelector('body');
      if (body && body.textContent.trim()) {
        /* Clone body children into a wrapper div */
        var wrapper = document.createElement('div');
        wrapper.className = 'rendered-html';
        while (body.firstChild) {
          wrapper.appendChild(body.firstChild);
        }
        container.innerHTML = '';
        container.appendChild(wrapper);
        container.classList.add('show');
        return;
      }

      container.classList.remove('show');
    } catch (e) {
      container.classList.remove('show');
    }
  }

  /* ---- Mobile nav toggle ---- */
  var toggle = $('navToggle');
  var links  = $('navLinks');
  if (toggle && links) {
    toggle.addEventListener('click', function () {
      toggle.classList.toggle('open');
      links.classList.toggle('open');
    });
    links.querySelectorAll('a').forEach(function (a) {
      a.addEventListener('click', function () {
        toggle.classList.remove('open');
        links.classList.remove('open');
      });
    });
  }

  /* ---- Smooth scroll for hash links ---- */
  document.querySelectorAll('a[href^="#"]').forEach(function (a) {
    a.addEventListener('click', function (e) {
      var target = document.querySelector(this.getAttribute('href'));
      if (target) {
        e.preventDefault();
        target.scrollIntoView({ behavior: 'smooth' });
      }
    });
  });

  /* ================================================================
     1. 静态资源 Demo
     ================================================================ */
  document.querySelectorAll('[data-demo="static"]').forEach(function (btn) {
    btn.addEventListener('click', function () {
      var path = this.getAttribute('data-path');
      fetchAndDisplay(path, {}, $('out-static'), null);
    });
  });

  /* ================================================================
     2. GET 查询 Demo (raw + table rendering)
     ================================================================ */
  var btnGetSearch = $('btnGetSearch');
  if (btnGetSearch) {
    btnGetSearch.addEventListener('click', function () {
      var cls = encodeURIComponent($('getClass').value.trim());
      var kw  = encodeURIComponent($('getKeyword').value.trim());
      var url = '/search?class=' + cls + '&keyword=' + kw;
      fetchAndDisplay(url, {}, $('out-get-raw'), $('out-get-table'));
    });
  }

  /* ================================================================
     3. POST 查询 Demo (raw + table rendering)
     ================================================================ */
  var btnPostSearch = $('btnPostSearch');
  if (btnPostSearch) {
    btnPostSearch.addEventListener('click', function () {
      var cls = $('postClass').value.trim();
      var kw  = $('postKeyword').value.trim();
      var body = 'class=' + encodeURIComponent(cls) + '&keyword=' + encodeURIComponent(kw);
      fetchAndDisplay('/search', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body
      }, $('out-post-raw'), $('out-post-table'));
    });
  }

  /* ================================================================
     4. 路由配置 Demo (with HTML rendering)
     ================================================================ */
  document.querySelectorAll('[data-demo="route"]').forEach(function (btn) {
    btn.addEventListener('click', function () {
      var path = this.getAttribute('data-path');
      fetchAndDisplay(path, {}, $('out-route-raw'), $('out-route-table'));
    });
  });

  /* POST /echo with custom body */
  var btnPostEcho = $('btnPostEcho');
  if (btnPostEcho) {
    btnPostEcho.addEventListener('click', function () {
      var body = $('echoBody').value.trim();
      fetchAndDisplay('/echo', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: body
      }, $('out-route-raw'), $('out-route-table'));
    });
  }

  /* ================================================================
     5. 404 Demo
     ================================================================ */
  document.querySelectorAll('[data-demo="error404"]').forEach(function (btn) {
    btn.addEventListener('click', function () {
      var path = this.getAttribute('data-path');
      fetchAndDisplay(path, {}, $('out-404'), null);
    });
  });

  /* ================================================================
     6. Basic 认证 Demo — 使用 /api/auth-check (永无 WWW-Authenticate)
     ================================================================ */
  var btnAuthLogin = $('btnAuthLogin');
  var outAuth      = $('out-auth');

  if (btnAuthLogin) {
    btnAuthLogin.addEventListener('click', function () {
      var user = $('authUser').value.trim();
      var pass = $('authPass').value.trim();

      if (!user && !pass) {
        /* 无凭据 */
        fetchAndDisplay('/api/auth-check', {}, outAuth, null);
      } else {
        var cred = btoa((user || 'student') + ':' + (pass || ''));
        fetchAndDisplay('/api/auth-check', {
          method: 'GET',
          headers: { 'Authorization': 'Basic ' + cred }
        }, outAuth, null);
      }
    });
  }

  /* Enter key in password field → trigger login */
  var authPassInput = $('authPass');
  if (authPassInput) {
    authPassInput.addEventListener('keydown', function (e) {
      if (e.key === 'Enter' && btnAuthLogin) btnAuthLogin.click();
    });
  }

  /* ================================================================
     7. 日志刷新 Demo
     ================================================================ */
  var btnLogRefresh = $('btnLogRefresh');
  var outLog        = $('out-log');
  var logStatus     = $('logStatus');

  if (btnLogRefresh) {
    btnLogRefresh.addEventListener('click', function () {
      if (logStatus) logStatus.textContent = '加载中...';
      outLog.textContent = 'Loading...';
      outLog.classList.add('show');
      outLog.classList.remove('error');

      fetch('/api/logs')
        .then(function (res) {
          return res.text().then(function (body) {
            var lines = [];
            lines.push('HTTP ' + res.status + ' ' + res.statusText);
            lines.push('');
            lines.push(body);
            showRawOutput(outLog, lines.join('\n'), res.status >= 400);
            if (logStatus) {
              var now = new Date();
              logStatus.textContent = '已刷新 ' + now.toLocaleTimeString();
            }
          });
        })
        .catch(function (err) {
          showRawOutput(outLog, 'Request failed:\n' + err.message, true);
          if (logStatus) logStatus.textContent = '刷新失败';
        });
    });

    /* Auto-load on page ready */
    btnLogRefresh.click();
  }

})();
