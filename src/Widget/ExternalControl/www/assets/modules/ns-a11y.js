// ns-a11y.js —— 表单可访问性绑定与运行时 ARIA 修正
(function() {
  'use strict';

  let seq = 0;

  function nextId(prefix) {
    seq += 1;
    return String(prefix || 'ns') + '_' + seq;
  }

  function isForLabelable(control) {
    if (!control || !control.tagName) return false;
    const tag = control.tagName.toUpperCase();
    if (tag === 'SELECT' || tag === 'TEXTAREA') return true;
    if (tag === 'INPUT') {
      const t = String(control.type || 'text').toLowerCase();
      return t !== 'hidden' && t !== 'color';
    }
    return false;
  }

  function ensureLabelId(labelEl) {
    if (!labelEl) return '';
    if (!labelEl.id) labelEl.id = nextId('ns_lbl');
    return labelEl.id;
  }

  function demoteOrphanLabel(labelNode) {
    if (!labelNode || labelNode.tagName !== 'LABEL') return labelNode;
    const hasFor = String(labelNode.getAttribute('for') || '').trim();
    const hasNestedControl = !!labelNode.querySelector('input,select,textarea,button');
    if (hasFor || hasNestedControl) return labelNode;
    const span = document.createElement('span');
    span.className = labelNode.className;
    span.textContent = labelNode.textContent;
    if (labelNode.id) span.id = labelNode.id;
    labelNode.replaceWith(span);
    return span;
  }

  function bindControl(labelText, control, opts) {
    if (!control) return control;
    const text = String(labelText || '').trim();
    const id = control.id || nextId('ns_ctrl');
    control.id = id;

    const labelEl = opts && opts.labelEl;
    let labelNode = labelEl;

    if (labelEl && labelEl.tagName === 'DIV') {
      const lbl = document.createElement('label');
      lbl.className = labelEl.className;
      lbl.textContent = labelEl.textContent;
      labelEl.replaceWith(lbl);
      labelNode = lbl;
    }

    if (labelNode && labelNode.tagName === 'LABEL') {
      if (isForLabelable(control)) {
        labelNode.htmlFor = id;
        try { control.removeAttribute('aria-label'); } catch {}
      } else {
        labelNode.removeAttribute('for');
        labelNode = demoteOrphanLabel(labelNode);
        const labelId = ensureLabelId(labelNode);
        const host = control.parentElement || control;
        try {
          host.setAttribute('role', 'group');
          host.setAttribute('aria-labelledby', labelId);
        } catch {}
        if (text) control.setAttribute('aria-label', text);
      }
    } else if (text && !control.getAttribute('aria-label') && !control.getAttribute('aria-labelledby')) {
      control.setAttribute('aria-label', text);
    }

    if (control.tagName === 'SELECT' && text && !control.getAttribute('title')) {
      control.setAttribute('title', text);
    }
    return control;
  }

  function wirePropRow(row, input, labelText) {
    if (!row || !input) return input;
    const labelEl = row.querySelector('.prop-label, label.prop-label');
    return bindControl(labelText || (labelEl ? labelEl.textContent : ''), input, { labelEl });
  }

  function fixColorPickerLabels(root) {
    try {
      const scope = root && root.querySelectorAll ? root : document;
      scope.querySelectorAll('input[type="hidden"][data-alpha-upgraded="1"]').forEach(inp => {
        const cid = inp.id;
        if (!cid) return;
        const row = inp.closest('.mb-1, .mb-2, .d-flex');
        const label = row
          ? row.querySelector('label[for="' + cid.replace(/"/g, '\\"') + '"], label.prop-label')
          : document.querySelector('label[for="' + cid.replace(/"/g, '\\"') + '"]');
        if (label) {
          label.removeAttribute('for');
          demoteOrphanLabel(label);
        }
        const ctl = inp.parentElement;
        if (label && ctl) {
          const labelId = ensureLabelId(label);
          try {
            ctl.setAttribute('role', 'group');
            ctl.setAttribute('aria-labelledby', labelId);
          } catch {}
        }
        const picker = ctl && ctl.querySelector ? ctl.querySelector('.el-color-picker') : null;
        if (picker && label) {
          const labelId = ensureLabelId(label);
          try {
            picker.setAttribute('role', 'group');
            picker.setAttribute('aria-labelledby', labelId);
          } catch {}
        }
      });
    } catch {}
  }

  function fixUnassociatedLabels() {
    try {
      document.querySelectorAll('label').forEach(label => {
        const fid = String(label.getAttribute('for') || '').trim();
        const hasNested = !!label.querySelector('input,select,textarea,button');
        if (hasNested) return;
        if (fid) {
          const target = document.getElementById(fid);
          if (target && target.type !== 'hidden') return;
        }
        demoteOrphanLabel(label);
      });
    } catch {}
  }

  function fixOrphanLabels() {
    try {
      document.querySelectorAll('label[for]').forEach(label => {
        const fid = String(label.getAttribute('for') || '').trim();
        if (!fid) return;
        const target = document.getElementById(fid);
        if (!target || target.type === 'hidden') label.removeAttribute('for');
      });
    } catch {}
  }

  function fixRuntimeA11y() {
    try {
      document.querySelectorAll('[aria-description]').forEach(el => {
        const desc = el.getAttribute('aria-description');
        el.removeAttribute('aria-description');
        if (desc && !el.getAttribute('aria-label') && !el.getAttribute('aria-labelledby')) {
          el.setAttribute('aria-label', desc);
        }
      });
    } catch {}

    try {
      document.querySelectorAll('iframe:not([title]), frame:not([title])').forEach((el, i) => {
        el.setAttribute('title', el.getAttribute('aria-label') || ('嵌入内容 ' + String(i + 1)));
      });
    } catch {}

    fixOrphanLabels();
    fixUnassociatedLabels();
    fixColorPickerLabels(document);
  }

  function observeDynamicA11y() {
    fixRuntimeA11y();
    let timer = 0;
    const obs = new MutationObserver(() => {
      if (timer) return;
      timer = setTimeout(() => {
        timer = 0;
        fixRuntimeA11y();
      }, 250);
    });
    if (document.body) {
      obs.observe(document.body, {
        childList: true,
        subtree: true,
        attributes: true,
        attributeFilter: ['aria-description', 'type', 'data-alpha-upgraded']
      });
    }
  }

  window.NSA11y = {
    nextId,
    bindControl,
    wirePropRow,
    fixColorPickerLabels,
    fixOrphanLabels,
    fixRuntimeA11y,
    observeDynamicA11y
  };
})();
