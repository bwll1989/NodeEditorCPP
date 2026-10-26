const __vite__mapDeps=(i,m=__vite__mapDeps,d=(m.f||(m.f=["./hui-line3d-card-BuCWIuVa.js","./chart-3d-card-shared-CqA3UdYl.js","./echarts-DsuMhSbC.js","./hui-scatter3d-card-BGC5RYAw.js","./hui-line2d-card-CaMj3KM_.js","./chart-2d-card-shared-D-hxRXmN.js","./hui-scatter2d-card-BN980qKi.js","./hui-bar-card-CDc7csu6.js"])))=>i.map(i=>d[i]);
var Lu=Object.defineProperty;var Du=(e,t,i)=>t in e?Lu(e,t,{enumerable:!0,configurable:!0,writable:!0,value:i}):e[t]=i;var N=(e,t,i)=>Du(e,typeof t!="symbol"?t+"":t,i);(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const o of document.querySelectorAll('link[rel="modulepreload"]'))r(o);new MutationObserver(o=>{for(const n of o)if(n.type==="childList")for(const s of n.addedNodes)s.tagName==="LINK"&&s.rel==="modulepreload"&&r(s)}).observe(document,{childList:!0,subtree:!0});function i(o){const n={};return o.integrity&&(n.integrity=o.integrity),o.referrerPolicy&&(n.referrerPolicy=o.referrerPolicy),o.crossOrigin==="use-credentials"?n.credentials="include":o.crossOrigin==="anonymous"?n.credentials="omit":n.credentials="same-origin",n}function r(o){if(o.ep)return;o.ep=!0;const n=i(o);fetch(o.href,n)}})();const Iu="modulepreload",Hu=function(e,t){return new URL(e,t).href},Pc={},Sr=function(t,i,r){let o=Promise.resolve();if(i&&i.length>0){const s=document.getElementsByTagName("link"),a=document.querySelector("meta[property=csp-nonce]"),l=(a==null?void 0:a.nonce)||(a==null?void 0:a.getAttribute("nonce"));o=Promise.allSettled(i.map(c=>{if(c=Hu(c,r),c in Pc)return;Pc[c]=!0;const d=c.endsWith(".css"),p=d?'[rel="stylesheet"]':"";if(!!r)for(let v=s.length-1;v>=0;v--){const _=s[v];if(_.href===c&&(!d||_.rel==="stylesheet"))return}else if(document.querySelector(`link[href="${c}"]${p}`))return;const b=document.createElement("link");if(b.rel=d?"stylesheet":Iu,d||(b.as="script"),b.crossOrigin="",b.href=c,l&&b.setAttribute("nonce",l),document.head.appendChild(b),d)return new Promise((v,_)=>{b.addEventListener("load",v),b.addEventListener("error",()=>_(new Error(`Unable to preload CSS for ${c}`)))})}))}function n(s){const a=new Event("vite:preloadError",{cancelable:!0});if(a.payload=s,window.dispatchEvent(a),!a.defaultPrevented)throw s}return o.then(s=>{for(const a of s||[])a.status==="rejected"&&n(a.reason);return t().catch(n)})},zs=new Map;function B(e){zs.set(e.type,e)}function fh(e){return zs.get(e)}function zu(){return[...zs.values()]}function Ac(e){return Object.hasOwn(e,"constructor")&&typeof e.constructor=="function"}async function ml(e){const t=zs.get(e);if(t)return Ac(t)?t.constructor:(t.loader&&await t.loader(),Ac(t)?t.constructor:void 0)}function gh(){return zu().filter(e=>e.type!=="error"&&e.type!=="color")}/**
 * @license
 * Copyright 2019 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const Yn=globalThis,bl=Yn.ShadowRoot&&(Yn.ShadyCSS===void 0||Yn.ShadyCSS.nativeShadow)&&"adoptedStyleSheets"in Document.prototype&&"replace"in CSSStyleSheet.prototype,vl=Symbol(),Tc=new WeakMap;let mh=class{constructor(t,i,r){if(this._$cssResult$=!0,r!==vl)throw Error("CSSResult is not constructable. Use `unsafeCSS` or `css` instead.");this.cssText=t,this.t=i}get styleSheet(){let t=this.o;const i=this.t;if(bl&&t===void 0){const r=i!==void 0&&i.length===1;r&&(t=Tc.get(i)),t===void 0&&((this.o=t=new CSSStyleSheet).replaceSync(this.cssText),r&&Tc.set(i,t))}return t}toString(){return this.cssText}};const Vu=e=>new mh(typeof e=="string"?e:e+"",void 0,vl),w=(e,...t)=>{const i=e.length===1?e[0]:t.reduce((r,o,n)=>r+(s=>{if(s._$cssResult$===!0)return s.cssText;if(typeof s=="number")return s;throw Error("Value passed to 'css' function must be a 'css' function result: "+s+". Use 'unsafeCSS' to pass non-literal values, but take care to ensure page security.")})(o)+e[n+1],e[0]);return new mh(i,e,vl)},Nu=(e,t)=>{if(bl)e.adoptedStyleSheets=t.map(i=>i instanceof CSSStyleSheet?i:i.styleSheet);else for(const i of t){const r=document.createElement("style"),o=Yn.litNonce;o!==void 0&&r.setAttribute("nonce",o),r.textContent=i.cssText,e.appendChild(r)}},Mc=bl?e=>e:e=>e instanceof CSSStyleSheet?(t=>{let i="";for(const r of t.cssRules)i+=r.cssText;return Vu(i)})(e):e;/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const{is:Ru,defineProperty:Fu,getOwnPropertyDescriptor:Bu,getOwnPropertyNames:Uu,getOwnPropertySymbols:ju,getPrototypeOf:qu}=Object,li=globalThis,Oc=li.trustedTypes,Gu=Oc?Oc.emptyScript:"",na=li.reactiveElementPolyfillSupport,To=(e,t)=>e,ls={toAttribute(e,t){switch(t){case Boolean:e=e?Gu:null;break;case Object:case Array:e=e==null?e:JSON.stringify(e)}return e},fromAttribute(e,t){let i=e;switch(t){case Boolean:i=e!==null;break;case Number:i=e===null?null:Number(e);break;case Object:case Array:try{i=JSON.parse(e)}catch{i=null}}return i}},_l=(e,t)=>!Ru(e,t),Lc={attribute:!0,type:String,converter:ls,reflect:!1,useDefault:!1,hasChanged:_l};Symbol.metadata??(Symbol.metadata=Symbol("metadata")),li.litPropertyMetadata??(li.litPropertyMetadata=new WeakMap);let Di=class extends HTMLElement{static addInitializer(t){this._$Ei(),(this.l??(this.l=[])).push(t)}static get observedAttributes(){return this.finalize(),this._$Eh&&[...this._$Eh.keys()]}static createProperty(t,i=Lc){if(i.state&&(i.attribute=!1),this._$Ei(),this.prototype.hasOwnProperty(t)&&((i=Object.create(i)).wrapped=!0),this.elementProperties.set(t,i),!i.noAccessor){const r=Symbol(),o=this.getPropertyDescriptor(t,r,i);o!==void 0&&Fu(this.prototype,t,o)}}static getPropertyDescriptor(t,i,r){const{get:o,set:n}=Bu(this.prototype,t)??{get(){return this[i]},set(s){this[i]=s}};return{get:o,set(s){const a=o==null?void 0:o.call(this);n==null||n.call(this,s),this.requestUpdate(t,a,r)},configurable:!0,enumerable:!0}}static getPropertyOptions(t){return this.elementProperties.get(t)??Lc}static _$Ei(){if(this.hasOwnProperty(To("elementProperties")))return;const t=qu(this);t.finalize(),t.l!==void 0&&(this.l=[...t.l]),this.elementProperties=new Map(t.elementProperties)}static finalize(){if(this.hasOwnProperty(To("finalized")))return;if(this.finalized=!0,this._$Ei(),this.hasOwnProperty(To("properties"))){const i=this.properties,r=[...Uu(i),...ju(i)];for(const o of r)this.createProperty(o,i[o])}const t=this[Symbol.metadata];if(t!==null){const i=litPropertyMetadata.get(t);if(i!==void 0)for(const[r,o]of i)this.elementProperties.set(r,o)}this._$Eh=new Map;for(const[i,r]of this.elementProperties){const o=this._$Eu(i,r);o!==void 0&&this._$Eh.set(o,i)}this.elementStyles=this.finalizeStyles(this.styles)}static finalizeStyles(t){const i=[];if(Array.isArray(t)){const r=new Set(t.flat(1/0).reverse());for(const o of r)i.unshift(Mc(o))}else t!==void 0&&i.push(Mc(t));return i}static _$Eu(t,i){const r=i.attribute;return r===!1?void 0:typeof r=="string"?r:typeof t=="string"?t.toLowerCase():void 0}constructor(){super(),this._$Ep=void 0,this.isUpdatePending=!1,this.hasUpdated=!1,this._$Em=null,this._$Ev()}_$Ev(){var t;this._$ES=new Promise(i=>this.enableUpdating=i),this._$AL=new Map,this._$E_(),this.requestUpdate(),(t=this.constructor.l)==null||t.forEach(i=>i(this))}addController(t){var i;(this._$EO??(this._$EO=new Set)).add(t),this.renderRoot!==void 0&&this.isConnected&&((i=t.hostConnected)==null||i.call(t))}removeController(t){var i;(i=this._$EO)==null||i.delete(t)}_$E_(){const t=new Map,i=this.constructor.elementProperties;for(const r of i.keys())this.hasOwnProperty(r)&&(t.set(r,this[r]),delete this[r]);t.size>0&&(this._$Ep=t)}createRenderRoot(){const t=this.shadowRoot??this.attachShadow(this.constructor.shadowRootOptions);return Nu(t,this.constructor.elementStyles),t}connectedCallback(){var t;this.renderRoot??(this.renderRoot=this.createRenderRoot()),this.enableUpdating(!0),(t=this._$EO)==null||t.forEach(i=>{var r;return(r=i.hostConnected)==null?void 0:r.call(i)})}enableUpdating(t){}disconnectedCallback(){var t;(t=this._$EO)==null||t.forEach(i=>{var r;return(r=i.hostDisconnected)==null?void 0:r.call(i)})}attributeChangedCallback(t,i,r){this._$AK(t,r)}_$ET(t,i){var n;const r=this.constructor.elementProperties.get(t),o=this.constructor._$Eu(t,r);if(o!==void 0&&r.reflect===!0){const s=(((n=r.converter)==null?void 0:n.toAttribute)!==void 0?r.converter:ls).toAttribute(i,r.type);this._$Em=t,s==null?this.removeAttribute(o):this.setAttribute(o,s),this._$Em=null}}_$AK(t,i){var n,s;const r=this.constructor,o=r._$Eh.get(t);if(o!==void 0&&this._$Em!==o){const a=r.getPropertyOptions(o),l=typeof a.converter=="function"?{fromAttribute:a.converter}:((n=a.converter)==null?void 0:n.fromAttribute)!==void 0?a.converter:ls;this._$Em=o;const c=l.fromAttribute(i,a.type);this[o]=c??((s=this._$Ej)==null?void 0:s.get(o))??c,this._$Em=null}}requestUpdate(t,i,r,o=!1,n){var s;if(t!==void 0){const a=this.constructor;if(o===!1&&(n=this[t]),r??(r=a.getPropertyOptions(t)),!((r.hasChanged??_l)(n,i)||r.useDefault&&r.reflect&&n===((s=this._$Ej)==null?void 0:s.get(t))&&!this.hasAttribute(a._$Eu(t,r))))return;this.C(t,i,r)}this.isUpdatePending===!1&&(this._$ES=this._$EP())}C(t,i,{useDefault:r,reflect:o,wrapped:n},s){r&&!(this._$Ej??(this._$Ej=new Map)).has(t)&&(this._$Ej.set(t,s??i??this[t]),n!==!0||s!==void 0)||(this._$AL.has(t)||(this.hasUpdated||r||(i=void 0),this._$AL.set(t,i)),o===!0&&this._$Em!==t&&(this._$Eq??(this._$Eq=new Set)).add(t))}async _$EP(){this.isUpdatePending=!0;try{await this._$ES}catch(i){Promise.reject(i)}const t=this.scheduleUpdate();return t!=null&&await t,!this.isUpdatePending}scheduleUpdate(){return this.performUpdate()}performUpdate(){var r;if(!this.isUpdatePending)return;if(!this.hasUpdated){if(this.renderRoot??(this.renderRoot=this.createRenderRoot()),this._$Ep){for(const[n,s]of this._$Ep)this[n]=s;this._$Ep=void 0}const o=this.constructor.elementProperties;if(o.size>0)for(const[n,s]of o){const{wrapped:a}=s,l=this[n];a!==!0||this._$AL.has(n)||l===void 0||this.C(n,void 0,s,l)}}let t=!1;const i=this._$AL;try{t=this.shouldUpdate(i),t?(this.willUpdate(i),(r=this._$EO)==null||r.forEach(o=>{var n;return(n=o.hostUpdate)==null?void 0:n.call(o)}),this.update(i)):this._$EM()}catch(o){throw t=!1,this._$EM(),o}t&&this._$AE(i)}willUpdate(t){}_$AE(t){var i;(i=this._$EO)==null||i.forEach(r=>{var o;return(o=r.hostUpdated)==null?void 0:o.call(r)}),this.hasUpdated||(this.hasUpdated=!0,this.firstUpdated(t)),this.updated(t)}_$EM(){this._$AL=new Map,this.isUpdatePending=!1}get updateComplete(){return this.getUpdateComplete()}getUpdateComplete(){return this._$ES}shouldUpdate(t){return!0}update(t){this._$Eq&&(this._$Eq=this._$Eq.forEach(i=>this._$ET(i,this[i]))),this._$EM()}updated(t){}firstUpdated(t){}};Di.elementStyles=[],Di.shadowRootOptions={mode:"open"},Di[To("elementProperties")]=new Map,Di[To("finalized")]=new Map,na==null||na({ReactiveElement:Di}),(li.reactiveElementVersions??(li.reactiveElementVersions=[])).push("2.1.2");/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const Mo=globalThis,Dc=e=>e,cs=Mo.trustedTypes,Ic=cs?cs.createPolicy("lit-html",{createHTML:e=>e}):void 0,bh="$lit$",ii=`lit$${Math.random().toFixed(9).slice(2)}$`,vh="?"+ii,Xu=`<${vh}>`,ji=document,Bo=()=>ji.createComment(""),Uo=e=>e===null||typeof e!="object"&&typeof e!="function",yl=Array.isArray,Wu=e=>yl(e)||typeof(e==null?void 0:e[Symbol.iterator])=="function",sa=`[ 	
\f\r]`,mo=/<(?:(!--|\/[^a-zA-Z])|(\/?[a-zA-Z][^>\s]*)|(\/?$))/g,Hc=/-->/g,zc=/>/g,Ai=RegExp(`>|${sa}(?:([^\\s"'>=/]+)(${sa}*=${sa}*(?:[^ 	
\f\r"'\`<>=]|("|')|))|$)`,"g"),Vc=/'/g,Nc=/"/g,_h=/^(?:script|style|textarea|title)$/i,Yu=e=>(t,...i)=>({_$litType$:e,strings:t,values:i}),h=Yu(1),Vt=Symbol.for("lit-noChange"),m=Symbol.for("lit-nothing"),Rc=new WeakMap,Hi=ji.createTreeWalker(ji,129);function yh(e,t){if(!yl(e)||!e.hasOwnProperty("raw"))throw Error("invalid template strings array");return Ic!==void 0?Ic.createHTML(t):t}const Zu=(e,t)=>{const i=e.length-1,r=[];let o,n=t===2?"<svg>":t===3?"<math>":"",s=mo;for(let a=0;a<i;a++){const l=e[a];let c,d,p=-1,f=0;for(;f<l.length&&(s.lastIndex=f,d=s.exec(l),d!==null);)f=s.lastIndex,s===mo?d[1]==="!--"?s=Hc:d[1]!==void 0?s=zc:d[2]!==void 0?(_h.test(d[2])&&(o=RegExp("</"+d[2],"g")),s=Ai):d[3]!==void 0&&(s=Ai):s===Ai?d[0]===">"?(s=o??mo,p=-1):d[1]===void 0?p=-2:(p=s.lastIndex-d[2].length,c=d[1],s=d[3]===void 0?Ai:d[3]==='"'?Nc:Vc):s===Nc||s===Vc?s=Ai:s===Hc||s===zc?s=mo:(s=Ai,o=void 0);const b=s===Ai&&e[a+1].startsWith("/>")?" ":"";n+=s===mo?l+Xu:p>=0?(r.push(c),l.slice(0,p)+bh+l.slice(p)+ii+b):l+ii+(p===-2?a:b)}return[yh(e,n+(e[i]||"<?>")+(t===2?"</svg>":t===3?"</math>":"")),r]};let Ha=class xh{constructor({strings:t,_$litType$:i},r){let o;this.parts=[];let n=0,s=0;const a=t.length-1,l=this.parts,[c,d]=Zu(t,i);if(this.el=xh.createElement(c,r),Hi.currentNode=this.el.content,i===2||i===3){const p=this.el.content.firstChild;p.replaceWith(...p.childNodes)}for(;(o=Hi.nextNode())!==null&&l.length<a;){if(o.nodeType===1){if(o.hasAttributes())for(const p of o.getAttributeNames())if(p.endsWith(bh)){const f=d[s++],b=o.getAttribute(p).split(ii),v=/([.?@])?(.*)/.exec(f);l.push({type:1,index:n,name:v[2],strings:b,ctor:v[1]==="."?Qu:v[1]==="?"?Ju:v[1]==="@"?ef:Vs}),o.removeAttribute(p)}else p.startsWith(ii)&&(l.push({type:6,index:n}),o.removeAttribute(p));if(_h.test(o.tagName)){const p=o.textContent.split(ii),f=p.length-1;if(f>0){o.textContent=cs?cs.emptyScript:"";for(let b=0;b<f;b++)o.append(p[b],Bo()),Hi.nextNode(),l.push({type:2,index:++n});o.append(p[f],Bo())}}}else if(o.nodeType===8)if(o.data===vh)l.push({type:2,index:n});else{let p=-1;for(;(p=o.data.indexOf(ii,p+1))!==-1;)l.push({type:7,index:n}),p+=ii.length-1}n++}}static createElement(t,i){const r=ji.createElement("template");return r.innerHTML=t,r}};function Lr(e,t,i=e,r){var s,a;if(t===Vt)return t;let o=r!==void 0?(s=i._$Co)==null?void 0:s[r]:i._$Cl;const n=Uo(t)?void 0:t._$litDirective$;return(o==null?void 0:o.constructor)!==n&&((a=o==null?void 0:o._$AO)==null||a.call(o,!1),n===void 0?o=void 0:(o=new n(e),o._$AT(e,i,r)),r!==void 0?(i._$Co??(i._$Co=[]))[r]=o:i._$Cl=o),o!==void 0&&(t=Lr(e,o._$AS(e,t.values),o,r)),t}let Ku=class{constructor(t,i){this._$AV=[],this._$AN=void 0,this._$AD=t,this._$AM=i}get parentNode(){return this._$AM.parentNode}get _$AU(){return this._$AM._$AU}u(t){const{el:{content:i},parts:r}=this._$AD,o=((t==null?void 0:t.creationScope)??ji).importNode(i,!0);Hi.currentNode=o;let n=Hi.nextNode(),s=0,a=0,l=r[0];for(;l!==void 0;){if(s===l.index){let c;l.type===2?c=new oo(n,n.nextSibling,this,t):l.type===1?c=new l.ctor(n,l.name,l.strings,this,t):l.type===6&&(c=new tf(n,this,t)),this._$AV.push(c),l=r[++a]}s!==(l==null?void 0:l.index)&&(n=Hi.nextNode(),s++)}return Hi.currentNode=ji,o}p(t){let i=0;for(const r of this._$AV)r!==void 0&&(r.strings!==void 0?(r._$AI(t,r,i),i+=r.strings.length-2):r._$AI(t[i])),i++}};class oo{get _$AU(){var t;return((t=this._$AM)==null?void 0:t._$AU)??this._$Cv}constructor(t,i,r,o){this.type=2,this._$AH=m,this._$AN=void 0,this._$AA=t,this._$AB=i,this._$AM=r,this.options=o,this._$Cv=(o==null?void 0:o.isConnected)??!0}get parentNode(){let t=this._$AA.parentNode;const i=this._$AM;return i!==void 0&&(t==null?void 0:t.nodeType)===11&&(t=i.parentNode),t}get startNode(){return this._$AA}get endNode(){return this._$AB}_$AI(t,i=this){t=Lr(this,t,i),Uo(t)?t===m||t==null||t===""?(this._$AH!==m&&this._$AR(),this._$AH=m):t!==this._$AH&&t!==Vt&&this._(t):t._$litType$!==void 0?this.$(t):t.nodeType!==void 0?this.T(t):Wu(t)?this.k(t):this._(t)}O(t){return this._$AA.parentNode.insertBefore(t,this._$AB)}T(t){this._$AH!==t&&(this._$AR(),this._$AH=this.O(t))}_(t){this._$AH!==m&&Uo(this._$AH)?this._$AA.nextSibling.data=t:this.T(ji.createTextNode(t)),this._$AH=t}$(t){var n;const{values:i,_$litType$:r}=t,o=typeof r=="number"?this._$AC(t):(r.el===void 0&&(r.el=Ha.createElement(yh(r.h,r.h[0]),this.options)),r);if(((n=this._$AH)==null?void 0:n._$AD)===o)this._$AH.p(i);else{const s=new Ku(o,this),a=s.u(this.options);s.p(i),this.T(a),this._$AH=s}}_$AC(t){let i=Rc.get(t.strings);return i===void 0&&Rc.set(t.strings,i=new Ha(t)),i}k(t){yl(this._$AH)||(this._$AH=[],this._$AR());const i=this._$AH;let r,o=0;for(const n of t)o===i.length?i.push(r=new oo(this.O(Bo()),this.O(Bo()),this,this.options)):r=i[o],r._$AI(n),o++;o<i.length&&(this._$AR(r&&r._$AB.nextSibling,o),i.length=o)}_$AR(t=this._$AA.nextSibling,i){var r;for((r=this._$AP)==null?void 0:r.call(this,!1,!0,i);t!==this._$AB;){const o=Dc(t).nextSibling;Dc(t).remove(),t=o}}setConnected(t){var i;this._$AM===void 0&&(this._$Cv=t,(i=this._$AP)==null||i.call(this,t))}}let Vs=class{get tagName(){return this.element.tagName}get _$AU(){return this._$AM._$AU}constructor(t,i,r,o,n){this.type=1,this._$AH=m,this._$AN=void 0,this.element=t,this.name=i,this._$AM=o,this.options=n,r.length>2||r[0]!==""||r[1]!==""?(this._$AH=Array(r.length-1).fill(new String),this.strings=r):this._$AH=m}_$AI(t,i=this,r,o){const n=this.strings;let s=!1;if(n===void 0)t=Lr(this,t,i,0),s=!Uo(t)||t!==this._$AH&&t!==Vt,s&&(this._$AH=t);else{const a=t;let l,c;for(t=n[0],l=0;l<n.length-1;l++)c=Lr(this,a[r+l],i,l),c===Vt&&(c=this._$AH[l]),s||(s=!Uo(c)||c!==this._$AH[l]),c===m?t=m:t!==m&&(t+=(c??"")+n[l+1]),this._$AH[l]=c}s&&!o&&this.j(t)}j(t){t===m?this.element.removeAttribute(this.name):this.element.setAttribute(this.name,t??"")}},Qu=class extends Vs{constructor(){super(...arguments),this.type=3}j(t){this.element[this.name]=t===m?void 0:t}},Ju=class extends Vs{constructor(){super(...arguments),this.type=4}j(t){this.element.toggleAttribute(this.name,!!t&&t!==m)}},ef=class extends Vs{constructor(t,i,r,o,n){super(t,i,r,o,n),this.type=5}_$AI(t,i=this){if((t=Lr(this,t,i,0)??m)===Vt)return;const r=this._$AH,o=t===m&&r!==m||t.capture!==r.capture||t.once!==r.once||t.passive!==r.passive,n=t!==m&&(r===m||o);o&&this.element.removeEventListener(this.name,this,r),n&&this.element.addEventListener(this.name,this,t),this._$AH=t}handleEvent(t){var i;typeof this._$AH=="function"?this._$AH.call(((i=this.options)==null?void 0:i.host)??this.element,t):this._$AH.handleEvent(t)}},tf=class{constructor(t,i,r){this.element=t,this.type=6,this._$AN=void 0,this._$AM=i,this.options=r}get _$AU(){return this._$AM._$AU}_$AI(t){Lr(this,t)}};const rf={I:oo},aa=Mo.litHtmlPolyfillSupport;aa==null||aa(Ha,oo),(Mo.litHtmlVersions??(Mo.litHtmlVersions=[])).push("3.3.3");const of=(e,t,i)=>{const r=(i==null?void 0:i.renderBefore)??t;let o=r._$litPart$;if(o===void 0){const n=(i==null?void 0:i.renderBefore)??null;r._$litPart$=o=new oo(t.insertBefore(Bo(),n),n,void 0,i??{})}return o._$AI(e),o};/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const Vi=globalThis;let y=class extends Di{constructor(){super(...arguments),this.renderOptions={host:this},this._$Do=void 0}createRenderRoot(){var i;const t=super.createRenderRoot();return(i=this.renderOptions).renderBefore??(i.renderBefore=t.firstChild),t}update(t){const i=this.render();this.hasUpdated||(this.renderOptions.isConnected=this.isConnected),super.update(t),this._$Do=of(i,this.renderRoot,this.renderOptions)}connectedCallback(){var t;super.connectedCallback(),(t=this._$Do)==null||t.setConnected(!0)}disconnectedCallback(){var t;super.disconnectedCallback(),(t=this._$Do)==null||t.setConnected(!1)}render(){return Vt}};var uh;y._$litElement$=!0,y.finalized=!0,(uh=Vi.litElementHydrateSupport)==null||uh.call(Vi,{LitElement:y});const la=Vi.litElementPolyfillSupport;la==null||la({LitElement:y});(Vi.litElementVersions??(Vi.litElementVersions=[])).push("4.2.2");/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const x=e=>(t,i)=>{i!==void 0?i.addInitializer(()=>{customElements.define(e,t)}):customElements.define(e,t)};/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const nf={attribute:!0,type:String,converter:ls,reflect:!1,hasChanged:_l},sf=(e=nf,t,i)=>{const{kind:r,metadata:o}=i;let n=globalThis.litPropertyMetadata.get(o);if(n===void 0&&globalThis.litPropertyMetadata.set(o,n=new Map),r==="setter"&&((e=Object.create(e)).wrapped=!0),n.set(i.name,e),r==="accessor"){const{name:s}=i;return{set(a){const l=t.get.call(this);t.set.call(this,a),this.requestUpdate(s,l,e,!0,a)},init(a){return a!==void 0&&this.C(s,void 0,e,a),a}}}if(r==="setter"){const{name:s}=i;return function(a){const l=this[s];t.call(this,a),this.requestUpdate(s,l,e,!0,a)}}throw Error("Unsupported decorator location: "+r)};function u(e){return(t,i)=>typeof i=="object"?sf(e,t,i):((r,o,n)=>{const s=o.hasOwnProperty(n);return o.constructor.createProperty(n,r),s?Object.getOwnPropertyDescriptor(o,n):void 0})(e,t,i)}/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */function g(e){return u({...e,state:!0,attribute:!1})}/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const af=(e,t,i)=>(i.configurable=!0,i.enumerable=!0,Reflect.decorate&&typeof t!="object"&&Object.defineProperty(e,t,i),i);/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */function mi(e,t){return(i,r,o)=>{const n=s=>{var a;return((a=s.renderRoot)==null?void 0:a.querySelector(e))??null};return af(i,r,{get(){return n(this)}})}}const Zn=50,wh={type:"heading",heading:"新建部件",heading_style:"title",badges:[{type:"button",entity:"",icon:"mdi:gesture-tap-button",text:"触发",show_name:!0,tap_action:{action:"trigger"},action:"trigger"}]};function $h(e){if(e!==void 0)return typeof e=="boolean"?e?{}:void 0:e}function kh(e=!0){return{type:"grid",cards:e?[{...wh}]:[]}}function Fc(e){return{title:(e==null?void 0:e.title)??"新建视图",path:(e==null?void 0:e.path)??"view",icon:(e==null?void 0:e.icon)??"mdi:view-dashboard",max_columns:4,sections:[kh(!0)]}}function lf(e){if(e.type==="button")return{...e,type:"trigger"};if(e.type==="color")return{...e,type:"rgba"};if(e.type==="bar"&&e.entity&&!e.entities){const{entity:t,name:i,...r}=e;return{...r,type:"bar",entities:[{entity:String(t),name:i?String(i):void 0}]}}return e}function cf(e){const{title:t,icon:i,badges:r,...o}=e,n=[...o.cards??[]].map(l=>lf(l)),s=!!(t||i||r!=null&&r.length),a=n.some(l=>l.type==="heading");return s&&!a&&n.unshift({type:"heading",heading:t||"新建部件",icon:i,badges:r==null?void 0:r.map(l=>({type:l.type??"entity",entity:l.entity,name:l.name,icon:l.icon}))}),{type:"grid",column_span:o.column_span,row_span:o.row_span,background:o.background,theme:o.theme,cards:n}}function Ch(e){return{...e,views:e.views.map(t=>df(t))}}function df(e){return{...e,sections:e.sections.map(t=>cf(t))}}function Dr(e){if(typeof e=="boolean"||typeof e=="number")return e;if(e==null)return!0;const t=String(e).trim();if(t===""||t==="true")return!0;if(t==="false")return!1;const i=Number(t);return t!==""&&Number.isFinite(i)&&String(i)===t?i:t}function Sh(e){return e&&typeof e=="object"?e:{}}function za(e,t){if(e===t)return!0;if(e===void 0||t===void 0||e===null||t===null)return!1;if(typeof e=="boolean"||typeof t=="boolean"){const i=n=>{if(typeof n=="boolean")return n;if(typeof n=="number")return n!==0;const s=String(n).trim().toLowerCase();if(s==="true"||s==="on"||s==="1")return!0;if(s==="false"||s==="off"||s==="0")return!1},r=i(e),o=i(t);return r!==void 0&&o!==void 0&&r===o}if(typeof e=="number"||typeof t=="number"){const i=Number(e),r=Number(t);return Number.isFinite(i)&&Number.isFinite(r)&&i===r}return String(e)===String(t)}function Bc(e,t){return za(e,t)}function Eh(e){return String(e??"entity")==="button"?"button":"entity"}function oi(e){return Eh(e==null?void 0:e.type)==="button"}function xl(e){return e==="switch"||e==="rw"||e==="toggle"}function hf(e,t,i){const r=String(e??"trigger");return r==="none"?"none":r==="open-card"?"open-card":r==="switch"?"switch":r==="rw"?"rw":r==="set"?"trigger":r==="toggle"?t!==void 0&&!za(t,!0)||i!==void 0&&!za(i,!1)?"rw":"switch":"trigger"}function pf(e){const t=e.card;return t&&typeof t=="object"?t:void 0}function jo(e){const t=e.tap_action&&typeof e.tap_action=="object"?e.tap_action:void 0,i=(t==null?void 0:t.press_value)!==void 0?t.press_value:e.press_value!==void 0?e.press_value:void 0,r=(t==null?void 0:t.release_value)!==void 0?t.release_value:e.release_value!==void 0?e.release_value:void 0,o=hf((t==null?void 0:t.action)??(typeof e.action=="string"?e.action:void 0),i,r);if(o==="none")return{action:"none"};if(o==="switch")return{action:"switch"};if(o==="rw")return{action:"rw",press_value:i===void 0?!0:Dr(i),release_value:r===void 0?!1:Dr(r)};if(o==="open-card"){const n=t!=null&&t.card&&typeof t.card=="object"?t.card:pf(e),s=String((n==null?void 0:n.type)??"slider").trim()||"slider",a=String((n==null?void 0:n.entity)??e.entity??"").trim()||String(e.entity??"").trim();return{action:"open-card",card:{...n&&typeof n=="object"?n:{},type:s,...a?{entity:a}:{}}}}return{action:"trigger"}}function Ge(e){const t=Sh(e),i=Eh(t.type),r=i==="button"?jo(t):{action:"none"},o={type:i,tap_action:r},n=String(t.entity??"").trim();n&&(o.entity=n);const s=String(t.name??"").trim();s&&(o.name=s);const a=String(t.icon??"").trim();a&&(o.icon=a);const l=String(t.color??"").trim();l&&(o.color=l);const c=String(t.unit??"").trim();c&&(o.unit=c);const d=String(t.text??"").trim();d&&(o.text=d),t.show_icon===!1&&(o.show_icon=!1),typeof t.show_name=="boolean"&&(o.show_name=t.show_name),typeof t.show_state=="boolean"&&(o.show_state=t.show_state),t.show_background===!1&&(o.show_background=!1);const p=Number(t.size);Number.isFinite(p)&&p>0&&(o.size=p);const f=String(t.attribute??"").trim();return f&&(o.attribute=f),t.style&&typeof t.style=="object"&&(o.style=t.style),o}function bi(e){const t=Ge(e),i=t.tap_action,r={type:t.type};if(t.entity&&(r.entity=t.entity),t.name&&(r.name=t.name),t.icon&&(r.icon=t.icon),t.color&&(r.color=t.color),t.unit&&(r.unit=t.unit),t.text&&(r.text=t.text),t.show_icon===!1&&(r.show_icon=!1),typeof t.show_name=="boolean"&&(r.show_name=t.show_name),typeof t.show_state=="boolean"&&(r.show_state=t.show_state),t.show_background===!1&&(r.show_background=!1),t.size!==void 0&&(r.size=t.size),t.attribute&&(r.attribute=t.attribute),t.style&&(r.style=t.style),t.type==="button"){const o={action:i.action};i.action==="rw"&&(o.press_value=i.press_value,o.release_value=i.release_value),i.action==="open-card"&&i.card&&(o.card=i.card),r.tap_action=o,r.action=i.action,i.press_value!==void 0&&(r.press_value=i.press_value),i.release_value!==void 0&&(r.release_value=i.release_value),i.card&&(r.card=i.card)}return r}function Kn(e){return bi({type:"entity",entity:"",icon:"mdi:thermometer",show_icon:!0,show_name:!0,...e})}function Va(e){return bi({type:"button",entity:"",icon:"mdi:gesture-tap-button",text:"触发",show_name:!0,tap_action:{action:"trigger"},...e})}function uf(e,t){const i=Ge(e),r=Sh(t),o=r.tap_action||r.action||r.value!==void 0||r.press_value!==void 0||r.release_value!==void 0||r.card!==void 0?jo({...i,...r,tap_action:r.tap_action??i.tap_action}):i.tap_action;return bi({...i,...r,tap_action:o})}const oe={"mdi:air-conditioner":"M6.59,0.66C8.93,-1.15 11.47,1.06 12.04,4.5C12.47,4.5 12.89,4.62 13.27,4.84C13.79,4.24 14.25,3.42 14.07,2.5C13.65,0.35 16.06,-1.39 18.35,1.58C20.16,3.92 17.95,6.46 14.5,7.03C14.5,7.46 14.39,7.89 14.16,8.27C14.76,8.78 15.58,9.24 16.5,9.06C18.63,8.64 20.38,11.04 17.41,13.34C15.07,15.15 12.53,12.94 11.96,9.5C11.53,9.5 11.11,9.37 10.74,9.15C10.22,9.75 9.75,10.58 9.93,11.5C10.35,13.64 7.94,15.39 5.65,12.42C3.83,10.07 6.05,7.53 9.5,6.97C9.5,6.54 9.63,6.12 9.85,5.74C9.25,5.23 8.43,4.76 7.5,4.94C5.37,5.36 3.62,2.96 6.59,0.66M5,16H7A2,2 0 0,1 9,18V24H7V22H5V24H3V18A2,2 0 0,1 5,16M5,18V20H7V18H5M12.93,16H15L12.07,24H10L12.93,16M18,16H21V18H18V22H21V24H18A2,2 0 0,1 16,22V18A2,2 0 0,1 18,16Z","mdi:alert-circle":"M13,13H11V7H13M13,17H11V15H13M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z","mdi:axis-arrow":"M12,2L16,6H13V13.85L19.53,17.61L21,15.03L22.5,20.5L17,21.96L18.53,19.35L12,15.58L5.47,19.35L7,21.96L1.5,20.5L3,15.03L4.47,17.61L11,13.85V6H8L12,2Z","mdi:bell":"M21,19V20H3V19L5,17V11C5,7.9 7.03,5.17 10,4.29C10,4.19 10,4.1 10,4A2,2 0 0,1 12,2A2,2 0 0,1 14,4C14,4.1 14,4.19 14,4.29C16.97,5.17 19,7.9 19,11V17L21,19M14,21A2,2 0 0,1 12,23A2,2 0 0,1 10,21","mdi:brightness-6":"M12,18V6A6,6 0 0,1 18,12A6,6 0 0,1 12,18M20,15.31L23.31,12L20,8.69V4H15.31L12,0.69L8.69,4H4V8.69L0.69,12L4,15.31V20H8.69L12,23.31L15.31,20H20V15.31Z","mdi:card-outline":"M20,20H4A2,2 0 0,1 2,18V6A2,2 0 0,1 4,4H20A2,2 0 0,1 22,6V18A2,2 0 0,1 20,20M4,6V18H20V6H4Z","mdi:cast-audio":"M2 11V13C7 13 11 17 11 22H13C13 15.9 8.1 11 2 11M20 2H10C8.9 2 8 2.9 8 4V10.5C9 11 9.9 11.7 10.7 12.4C11.6 11 13.2 10 15 10C17.8 10 20 12.2 20 15S17.8 20 15 20H14.8C14.9 20.7 15 21.3 15 22H20C21.1 22 22 21.1 22 20V4C22 2.9 21.1 2 20 2M15 8C13.9 8 13 7.1 13 6C13 4.9 13.9 4 15 4C16.1 4 17 4.9 17 6S16.1 8 15 8M15 18C14.8 18 14.5 18 14.3 17.9C13.8 16.4 13.1 15.1 12.2 13.9C12.6 12.8 13.7 11.9 15 11.9C16.7 11.9 18 13.2 18 14.9S16.7 18 15 18M2 15V17C4.8 17 7 19.2 7 22H9C9 18.1 5.9 15 2 15M2 19V22H5C5 20.3 3.7 19 2 19","mdi:chart-bar":"M22,21H2V3H4V19H6V10H10V19H12V6H16V19H18V14H22V21Z","mdi:chart-line":"M16,11.78L20.24,4.45L21.97,5.45L16.74,14.5L10.23,10.75L5.46,19H22V21H2V3H4V17.54L9.5,8L16,11.78Z","mdi:chart-scatter-plot":"M2,2H4V20H22V22H2V2M9,10A3,3 0 0,1 12,13A3,3 0 0,1 9,16A3,3 0 0,1 6,13A3,3 0 0,1 9,10M13,2A3,3 0 0,1 16,5A3,3 0 0,1 13,8A3,3 0 0,1 10,5A3,3 0 0,1 13,2M18,12A3,3 0 0,1 21,15A3,3 0 0,1 18,18A3,3 0 0,1 15,15A3,3 0 0,1 18,12Z","mdi:chart-scatter-plot-hexbin":"M2,2H4V20H22V22H2V2M14,14.5L12,18H7.94L5.92,14.5L7.94,11H12L14,14.5M14.08,6.5L12.06,10H8L6,6.5L8,3H12.06L14.08,6.5M21.25,10.5L19.23,14H15.19L13.17,10.5L15.19,7H19.23L21.25,10.5Z","mdi:chart-timeline-variant":"M3,14L3.5,14.07L8.07,9.5C7.89,8.85 8.06,8.11 8.59,7.59C9.37,6.8 10.63,6.8 11.41,7.59C11.94,8.11 12.11,8.85 11.93,9.5L14.5,12.07L15,12C15.18,12 15.35,12 15.5,12.07L19.07,8.5C19,8.35 19,8.18 19,8A2,2 0 0,1 21,6A2,2 0 0,1 23,8A2,2 0 0,1 21,10C20.82,10 20.65,10 20.5,9.93L16.93,13.5C17,13.65 17,13.82 17,14A2,2 0 0,1 15,16A2,2 0 0,1 13,14L13.07,13.5L10.5,10.93C10.18,11 9.82,11 9.5,10.93L4.93,15.5L5,16A2,2 0 0,1 3,18A2,2 0 0,1 1,16A2,2 0 0,1 3,14Z","mdi:check":"M21,7L9,19L3.5,13.5L4.91,12.09L9,16.17L19.59,5.59L21,7Z","mdi:clipboard-text-outline":"M19,3H14.82C14.25,1.44 12.53,0.64 11,1.2C10.14,1.5 9.5,2.16 9.18,3H5A2,2 0 0,0 3,5V19A2,2 0 0,0 5,21H19A2,2 0 0,0 21,19V5A2,2 0 0,0 19,3M12,3A1,1 0 0,1 13,4A1,1 0 0,1 12,5A1,1 0 0,1 11,4A1,1 0 0,1 12,3M7,7H17V5H19V19H5V5H7V7M17,11H7V9H17V11M15,15H7V13H15V15Z","mdi:clock-outline":"M12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22C6.47,22 2,17.5 2,12A10,10 0 0,1 12,2M12.5,7V12.25L17,14.92L16.25,16.15L11,13V7H12.5Z","mdi:close":"M19,6.41L17.59,5L12,10.59L6.41,5L5,6.41L10.59,12L5,17.59L6.41,19L12,13.41L17.59,19L19,17.59L13.41,12L19,6.41Z","mdi:coffee":"M2,21H20V19H2M20,8H18V5H20M20,3H4V13A4,4 0 0,0 8,17H14A4,4 0 0,0 18,13V10H20A2,2 0 0,0 22,8V5C22,3.89 21.1,3 20,3Z","mdi:cog":"M12,15.5A3.5,3.5 0 0,1 8.5,12A3.5,3.5 0 0,1 12,8.5A3.5,3.5 0 0,1 15.5,12A3.5,3.5 0 0,1 12,15.5M19.43,12.97C19.47,12.65 19.5,12.33 19.5,12C19.5,11.67 19.47,11.34 19.43,11L21.54,9.37C21.73,9.22 21.78,8.95 21.66,8.73L19.66,5.27C19.54,5.05 19.27,4.96 19.05,5.05L16.56,6.05C16.04,5.66 15.5,5.32 14.87,5.07L14.5,2.42C14.46,2.18 14.25,2 14,2H10C9.75,2 9.54,2.18 9.5,2.42L9.13,5.07C8.5,5.32 7.96,5.66 7.44,6.05L4.95,5.05C4.73,4.96 4.46,5.05 4.34,5.27L2.34,8.73C2.21,8.95 2.27,9.22 2.46,9.37L4.57,11C4.53,11.34 4.5,11.67 4.5,12C4.5,12.33 4.53,12.65 4.57,12.97L2.46,14.63C2.27,14.78 2.21,15.05 2.34,15.27L4.34,18.73C4.46,18.95 4.73,19.03 4.95,18.95L7.44,17.94C7.96,18.34 8.5,18.68 9.13,18.93L9.5,21.58C9.54,21.82 9.75,22 10,22H14C14.25,22 14.46,21.82 14.5,21.58L14.87,18.93C15.5,18.67 16.04,18.34 16.56,17.94L19.05,18.95C19.27,19.03 19.54,18.95 19.66,18.73L21.66,15.27C21.78,15.05 21.73,14.78 21.54,14.63L19.43,12.97Z","mdi:content-copy":"M19,21H8V7H19M19,5H8A2,2 0 0,0 6,7V21A2,2 0 0,0 8,23H19A2,2 0 0,0 21,21V7A2,2 0 0,0 19,5M16,1H4A2,2 0 0,0 2,3V17H4V3H16V1Z","mdi:content-cut":"M19,3L13,9L15,11L22,4V3M12,12.5A0.5,0.5 0 0,1 11.5,12A0.5,0.5 0 0,1 12,11.5A0.5,0.5 0 0,1 12.5,12A0.5,0.5 0 0,1 12,12.5M6,20A2,2 0 0,1 4,18C4,16.89 4.9,16 6,16A2,2 0 0,1 8,18C8,19.11 7.1,20 6,20M6,8A2,2 0 0,1 4,6C4,4.89 4.9,4 6,4A2,2 0 0,1 8,6C8,7.11 7.1,8 6,8M9.64,7.64C9.87,7.14 10,6.59 10,6A4,4 0 0,0 6,2A4,4 0 0,0 2,6A4,4 0 0,0 6,10C6.59,10 7.14,9.87 7.64,9.64L10,12L7.64,14.36C7.14,14.13 6.59,14 6,14A4,4 0 0,0 2,18A4,4 0 0,0 6,22A4,4 0 0,0 10,18C10,17.41 9.87,16.86 9.64,16.36L12,14L19,21H22V20L9.64,7.64Z","mdi:content-paste":"M19,20H5V4H7V7H17V4H19M12,2A1,1 0 0,1 13,3A1,1 0 0,1 12,4A1,1 0 0,1 11,3A1,1 0 0,1 12,2M19,2H14.82C14.4,0.84 13.3,0 12,0C10.7,0 9.6,0.84 9.18,2H5A2,2 0 0,0 3,4V20A2,2 0 0,0 5,22H19A2,2 0 0,0 21,20V4A2,2 0 0,0 19,2Z","mdi:cursor-move":"M13,6V11H18V7.75L22.25,12L18,16.25V13H13V18H16.25L12,22.25L7.75,18H11V13H6V16.25L1.75,12L6,7.75V11H11V6H7.75L12,1.75L16.25,6H13Z","mdi:delete":"M19,4H15.5L14.5,3H9.5L8.5,4H5V6H19M6,19A2,2 0 0,0 8,21H16A2,2 0 0,0 18,19V7H6V19Z","mdi:desk-lamp":"M10.85,2L9.18,4.5L10.32,5.25L7.14,10C7.1,10 7.05,10 7,10A2,2 0 0,0 5,12C5,12.94 5.66,13.75 6.58,13.95L10.62,20H7V22H17V20H13L8.53,13.28C8.83,12.92 9,12.47 9,12C9,11.7 8.93,11.4 8.8,11.13L12,6.37C11.78,8.05 12.75,9.89 14.45,11L18.89,4.37C17.2,3.24 15.12,3.04 13.65,3.87L10.85,2M18.33,7L16.67,9.5C17.35,9.95 18.29,9.77 18.75,9.08C19.21,8.39 19,7.46 18.33,7Z","mdi:devices":"M3 6H21V4H3C1.9 4 1 4.9 1 6V18C1 19.1 1.9 20 3 20H7V18H3V6M13 12H9V13.78C8.39 14.33 8 15.11 8 16C8 16.89 8.39 17.67 9 18.22V20H13V18.22C13.61 17.67 14 16.88 14 16S13.61 14.33 13 13.78V12M11 17.5C10.17 17.5 9.5 16.83 9.5 16S10.17 14.5 11 14.5 12.5 15.17 12.5 16 11.83 17.5 11 17.5M22 8H16C15.5 8 15 8.5 15 9V19C15 19.5 15.5 20 16 20H22C22.5 20 23 19.5 23 19V9C23 8.5 22.5 8 22 8M21 18H17V10H21V18Z","mdi:dishwasher":"M18,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V4A2,2 0 0,0 18,2M10,4A1,1 0 0,1 11,5A1,1 0 0,1 10,6A1,1 0 0,1 9,5A1,1 0 0,1 10,4M7,4A1,1 0 0,1 8,5A1,1 0 0,1 7,6A1,1 0 0,1 6,5A1,1 0 0,1 7,4M18,20H6V8H18V20M14.67,15.33C14.69,16.03 14.41,16.71 13.91,17.21C12.86,18.26 11.15,18.27 10.09,17.21C9.59,16.71 9.31,16.03 9.33,15.33C9.4,14.62 9.63,13.94 10,13.33C10.37,12.5 10.81,11.73 11.33,11L12,10C13.79,12.59 14.67,14.36 14.67,15.33","mdi:dots-vertical":"M12,16A2,2 0 0,1 14,18A2,2 0 0,1 12,20A2,2 0 0,1 10,18A2,2 0 0,1 12,16M12,10A2,2 0 0,1 14,12A2,2 0 0,1 12,14A2,2 0 0,1 10,12A2,2 0 0,1 12,10M12,4A2,2 0 0,1 14,6A2,2 0 0,1 12,8A2,2 0 0,1 10,6A2,2 0 0,1 12,4Z","mdi:eye":"M12,9A3,3 0 0,0 9,12A3,3 0 0,0 12,15A3,3 0 0,0 15,12A3,3 0 0,0 12,9M12,17A5,5 0 0,1 7,12A5,5 0 0,1 12,7A5,5 0 0,1 17,12A5,5 0 0,1 12,17M12,4.5C7,4.5 2.73,7.61 1,12C2.73,16.39 7,19.5 12,19.5C17,19.5 21.27,16.39 23,12C21.27,7.61 17,4.5 12,4.5Z","mdi:filmstrip":"M18,9H16V7H18M18,13H16V11H18M18,17H16V15H18M8,9H6V7H8M8,13H6V11H8M8,17H6V15H8M18,3V5H16V3H8V5H6V3H4V21H6V19H8V21H16V19H18V21H20V3H18Z","mdi:flash":"M7,2V13H10V22L17,10H13L17,2H7Z","mdi:floor-lamp":"M15,2L17,9H7L9,2M11,10H13V20H16V22H8V20H11V10Z","mdi:floor-plan":"M10,5V10H9V5H5V13H9V12H10V17H9V14H5V19H12V17H13V19H19V17H21V21H3V3H21V15H19V10H13V15H12V9H19V5H10Z","mdi:format-title":"M5,4V7H10.5V19H13.5V7H19V4H5Z","mdi:fridge":"M7,2H17A2,2 0 0,1 19,4V9H5V4A2,2 0 0,1 7,2M19,19A2,2 0 0,1 17,21V22H15V21H9V22H7V21A2,2 0 0,1 5,19V10H19V19M8,5V7H10V5H8M8,12V15H10V12H8Z","mdi:gauge":"M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M12,4A8,8 0 0,1 20,12C20,14.4 19,16.5 17.3,18C15.9,16.7 14,16 12,16C10,16 8.2,16.7 6.7,18C5,16.5 4,14.4 4,12A8,8 0 0,1 12,4M14,5.89C13.62,5.9 13.26,6.15 13.1,6.54L11.81,9.77L11.71,10C11,10.13 10.41,10.6 10.14,11.26C9.73,12.29 10.23,13.45 11.26,13.86C12.29,14.27 13.45,13.77 13.86,12.74C14.12,12.08 14,11.32 13.57,10.76L13.67,10.5L14.96,7.29L14.97,7.26C15.17,6.75 14.92,6.17 14.41,5.96C14.28,5.91 14.15,5.89 14,5.89M10,6A1,1 0 0,0 9,7A1,1 0 0,0 10,8A1,1 0 0,0 11,7A1,1 0 0,0 10,6M7,9A1,1 0 0,0 6,10A1,1 0 0,0 7,11A1,1 0 0,0 8,10A1,1 0 0,0 7,9M17,9A1,1 0 0,0 16,10A1,1 0 0,0 17,11A1,1 0 0,0 18,10A1,1 0 0,0 17,9Z","mdi:gesture-tap-button":"M13 5C15.21 5 17 6.79 17 9C17 10.5 16.2 11.77 15 12.46V11.24C15.61 10.69 16 9.89 16 9C16 7.34 14.66 6 13 6S10 7.34 10 9C10 9.89 10.39 10.69 11 11.24V12.46C9.8 11.77 9 10.5 9 9C9 6.79 10.79 5 13 5M20 20.5C19.97 21.32 19.32 21.97 18.5 22H13C12.62 22 12.26 21.85 12 21.57L8 17.37L8.74 16.6C8.93 16.39 9.2 16.28 9.5 16.28H9.7L12 18V9C12 8.45 12.45 8 13 8S14 8.45 14 9V13.47L15.21 13.6L19.15 15.79C19.68 16.03 20 16.56 20 17.14V20.5M20 2H4C2.9 2 2 2.9 2 4V12C2 13.11 2.9 14 4 14H8V12L4 12L4 4H20L20 12H18V14H20V13.96L20.04 14C21.13 14 22 13.09 22 12V4C22 2.9 21.11 2 20 2Z","mdi:hand-wave":"M23 17C23 20.31 20.31 23 17 23V21.5C19.5 21.5 21.5 19.5 21.5 17H23M1 7C1 3.69 3.69 1 7 1V2.5C4.5 2.5 2.5 4.5 2.5 7H1M8 4.32L3.41 8.92C.19 12.14 .19 17.37 3.41 20.59S11.86 23.81 15.08 20.59L22.15 13.5C22.64 13.03 22.64 12.24 22.15 11.75C21.66 11.26 20.87 11.26 20.38 11.75L15.96 16.17L15.25 15.46L21.79 8.92C22.28 8.43 22.28 7.64 21.79 7.15S20.5 6.66 20 7.15L14.19 13L13.5 12.27L20.37 5.38C20.86 4.89 20.86 4.1 20.37 3.61S19.09 3.12 18.6 3.61L11.71 10.5L11 9.8L16.5 4.32C17 3.83 17 3.04 16.5 2.55S15.22 2.06 14.73 2.55L7.11 10.17C8.33 11.74 8.22 14 6.78 15.45L6.07 14.74C7.24 13.57 7.24 11.67 6.07 10.5L5.72 10.15L9.79 6.08C10.28 5.59 10.28 4.8 9.79 4.31C9.29 3.83 8.5 3.83 8 4.32Z","mdi:help-circle":"M15.07,11.25L14.17,12.17C13.45,12.89 13,13.5 13,15H11V14.5C11,13.39 11.45,12.39 12.17,11.67L13.41,10.41C13.78,10.05 14,9.55 14,9C14,7.89 13.1,7 12,7A2,2 0 0,0 10,9H8A4,4 0 0,1 12,5A4,4 0 0,1 16,9C16,9.88 15.64,10.67 15.07,11.25M13,19H11V17H13M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12C22,6.47 17.5,2 12,2Z","mdi:help-circle-outline":"M11,18H13V16H11V18M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M12,20C7.59,20 4,16.41 4,12C4,7.59 7.59,4 12,4C16.41,4 20,7.59 20,12C20,16.41 16.41,20 12,20M12,6A4,4 0 0,0 8,10H10A2,2 0 0,1 12,8A2,2 0 0,1 14,10C14,12 11,11.75 11,15H13C13,12.75 16,12.5 16,10A4,4 0 0,0 12,6Z","mdi:home":"M10,20V14H14V20H19V12H22L12,3L2,12H5V20H10Z","mdi:home-thermometer":"M19 8C20.11 8 21 8.9 21 10V16.76C21.61 17.31 22 18.11 22 19C22 20.66 20.66 22 19 22C17.34 22 16 20.66 16 19C16 18.11 16.39 17.31 17 16.76V10C17 8.9 17.9 8 19 8M19 9C18.45 9 18 9.45 18 10V11H20V10C20 9.45 19.55 9 19 9M5 20V12H2L12 3L16.4 6.96C15.54 7.69 15 8.78 15 10V16C14.37 16.83 14 17.87 14 19L14.1 20H5Z","mdi:label-outline":"M16,17H5V7H16L19.55,12M17.63,5.84C17.27,5.33 16.67,5 16,5H5A2,2 0 0,0 3,7V17A2,2 0 0,0 5,19H16C16.67,19 17.27,18.66 17.63,18.15L22,12L17.63,5.84Z","mdi:lan-connect":"M4,1C2.89,1 2,1.89 2,3V7C2,8.11 2.89,9 4,9H1V11H13V9H10C11.11,9 12,8.11 12,7V3C12,1.89 11.11,1 10,1H4M4,3H10V7H4V3M3,13V18L3,20H10V18H5V13H3M14,13C12.89,13 12,13.89 12,15V19C12,20.11 12.89,21 14,21H11V23H23V21H20C21.11,21 22,20.11 22,19V15C22,13.89 21.11,13 20,13H14M14,15H20V19H14V15Z","mdi:lan-disconnect":"M4,1C2.89,1 2,1.89 2,3V7C2,8.11 2.89,9 4,9H1V11H13V9H10C11.11,9 12,8.11 12,7V3C12,1.89 11.11,1 10,1H4M4,3H10V7H4V3M14,13C12.89,13 12,13.89 12,15V19C12,20.11 12.89,21 14,21H11V23H23V21H20C21.11,21 22,20.11 22,19V15C22,13.89 21.11,13 20,13H14M3.88,13.46L2.46,14.88L4.59,17L2.46,19.12L3.88,20.54L6,18.41L8.12,20.54L9.54,19.12L7.41,17L9.54,14.88L8.12,13.46L6,15.59L3.88,13.46M14,15H20V19H14V15Z","mdi:lightbulb":"M12,2A7,7 0 0,0 5,9C5,11.38 6.19,13.47 8,14.74V17A1,1 0 0,0 9,18H15A1,1 0 0,0 16,17V14.74C17.81,13.47 19,11.38 19,9A7,7 0 0,0 12,2M9,21A1,1 0 0,0 10,22H14A1,1 0 0,0 15,21V20H9V21Z","mdi:lightbulb-outline":"M12,2A7,7 0 0,1 19,9C19,11.38 17.81,13.47 16,14.74V17A1,1 0 0,1 15,18H9A1,1 0 0,1 8,17V14.74C6.19,13.47 5,11.38 5,9A7,7 0 0,1 12,2M9,21V20H15V21A1,1 0 0,1 14,22H10A1,1 0 0,1 9,21M12,4A5,5 0 0,0 7,9C7,11.05 8.23,12.81 10,13.58V16H14V13.58C15.77,12.81 17,11.05 17,9A5,5 0 0,0 12,4Z","mdi:lightning-bolt":"M11 15H6L13 1V9H18L11 23V15Z","mdi:link-variant":"M10.59,13.41C11,13.8 11,14.44 10.59,14.83C10.2,15.22 9.56,15.22 9.17,14.83C7.22,12.88 7.22,9.71 9.17,7.76V7.76L12.71,4.22C14.66,2.27 17.83,2.27 19.78,4.22C21.73,6.17 21.73,9.34 19.78,11.29L18.29,12.78C18.3,11.96 18.17,11.14 17.89,10.36L18.36,9.88C19.54,8.71 19.54,6.81 18.36,5.64C17.19,4.46 15.29,4.46 14.12,5.64L10.59,9.17C9.41,10.34 9.41,12.24 10.59,13.41M13.41,9.17C13.8,8.78 14.44,8.78 14.83,9.17C16.78,11.12 16.78,14.29 14.83,16.24V16.24L11.29,19.78C9.34,21.73 6.17,21.73 4.22,19.78C2.27,17.83 2.27,14.66 4.22,12.71L5.71,11.22C5.7,12.04 5.83,12.86 6.11,13.65L5.64,14.12C4.46,15.29 4.46,17.19 5.64,18.36C6.81,19.54 8.71,19.54 9.88,18.36L13.41,14.83C14.59,13.66 14.59,11.76 13.41,10.59C13,10.2 13,9.56 13.41,9.17Z","mdi:magnify":"M9.5,3A6.5,6.5 0 0,1 16,9.5C16,11.11 15.41,12.59 14.44,13.73L14.71,14H15.5L20.5,19L19,20.5L14,15.5V14.71L13.73,14.44C12.59,15.41 11.11,16 9.5,16A6.5,6.5 0 0,1 3,9.5A6.5,6.5 0 0,1 9.5,3M9.5,5C7,5 5,7 5,9.5C5,12 7,14 9.5,14C12,14 14,12 14,9.5C14,7 12,5 9.5,5Z","mdi:map-marker":"M12,11.5A2.5,2.5 0 0,1 9.5,9A2.5,2.5 0 0,1 12,6.5A2.5,2.5 0 0,1 14.5,9A2.5,2.5 0 0,1 12,11.5M12,2A7,7 0 0,0 5,9C5,14.25 12,22 12,22C12,22 19,14.25 19,9A7,7 0 0,0 12,2Z","mdi:map-marker-path":"M18,15A3,3 0 0,1 21,18A3,3 0 0,1 18,21C16.69,21 15.58,20.17 15.17,19H14V17H15.17C15.58,15.83 16.69,15 18,15M18,17A1,1 0 0,0 17,18A1,1 0 0,0 18,19A1,1 0 0,0 19,18A1,1 0 0,0 18,17M18,8A1.43,1.43 0 0,0 19.43,6.57C19.43,5.78 18.79,5.14 18,5.14C17.21,5.14 16.57,5.78 16.57,6.57A1.43,1.43 0 0,0 18,8M18,2.57A4,4 0 0,1 22,6.57C22,9.56 18,14 18,14C18,14 14,9.56 14,6.57A4,4 0 0,1 18,2.57M8.83,17H10V19H8.83C8.42,20.17 7.31,21 6,21A3,3 0 0,1 3,18C3,16.69 3.83,15.58 5,15.17V14H7V15.17C7.85,15.47 8.53,16.15 8.83,17M6,17A1,1 0 0,0 5,18A1,1 0 0,0 6,19A1,1 0 0,0 7,18A1,1 0 0,0 6,17M6,3A3,3 0 0,1 9,6C9,7.31 8.17,8.42 7,8.83V10H5V8.83C3.83,8.42 3,7.31 3,6A3,3 0 0,1 6,3M6,5A1,1 0 0,0 5,6A1,1 0 0,0 6,7A1,1 0 0,0 7,6A1,1 0 0,0 6,5M11,19V17H13V19H11M7,13H5V11H7V13Z","mdi:menu":"M3,6H21V8H3V6M3,11H21V13H3V11M3,16H21V18H3V16Z","mdi:menu-open":"M21,15.61L19.59,17L14.58,12L19.59,7L21,8.39L17.44,12L21,15.61M3,6H16V8H3V6M3,13V11H13V13H3M3,18V16H16V18H3Z","mdi:microphone":"M12,2A3,3 0 0,1 15,5V11A3,3 0 0,1 12,14A3,3 0 0,1 9,11V5A3,3 0 0,1 12,2M19,11C19,14.53 16.39,17.44 13,17.93V21H11V17.93C7.61,17.44 5,14.53 5,11H7A5,5 0 0,0 12,16A5,5 0 0,0 17,11H19Z","mdi:molecule-co2":"M5,7A2,2 0 0,0 3,9V15A2,2 0 0,0 5,17H8V15H5V9H8V7H5M11,7A2,2 0 0,0 9,9V15A2,2 0 0,0 11,17H13A2,2 0 0,0 15,15V9A2,2 0 0,0 13,7H11M11,9H13V15H11V9M16,10.5V12H19V13.5H17.5A1.5,1.5 0 0,0 16,15V18H20.5V16.5H17.5V15H19A1.5,1.5 0 0,0 20.5,13.5V12A1.5,1.5 0 0,0 19,10.5H16Z","mdi:palette":"M17.5,12A1.5,1.5 0 0,1 16,10.5A1.5,1.5 0 0,1 17.5,9A1.5,1.5 0 0,1 19,10.5A1.5,1.5 0 0,1 17.5,12M14.5,8A1.5,1.5 0 0,1 13,6.5A1.5,1.5 0 0,1 14.5,5A1.5,1.5 0 0,1 16,6.5A1.5,1.5 0 0,1 14.5,8M9.5,8A1.5,1.5 0 0,1 8,6.5A1.5,1.5 0 0,1 9.5,5A1.5,1.5 0 0,1 11,6.5A1.5,1.5 0 0,1 9.5,8M6.5,12A1.5,1.5 0 0,1 5,10.5A1.5,1.5 0 0,1 6.5,9A1.5,1.5 0 0,1 8,10.5A1.5,1.5 0 0,1 6.5,12M12,3A9,9 0 0,0 3,12A9,9 0 0,0 12,21A1.5,1.5 0 0,0 13.5,19.5C13.5,19.11 13.35,18.76 13.11,18.5C12.88,18.23 12.73,17.88 12.73,17.5A1.5,1.5 0 0,1 14.23,16H16A5,5 0 0,0 21,11C21,6.58 16.97,3 12,3Z","mdi:palette-swatch":"M2.53,19.65L3.87,20.21V11.18L1.44,17.04C1.03,18.06 1.5,19.23 2.53,19.65M22.03,15.95L17.07,4C16.76,3.23 16.03,2.77 15.26,2.75C15,2.75 14.73,2.79 14.47,2.9L7.1,5.95C6.35,6.26 5.89,7 5.87,7.75C5.86,8 5.91,8.29 6,8.55L11,20.5C11.29,21.28 12.03,21.74 12.81,21.75C13.07,21.75 13.33,21.7 13.58,21.6L20.94,18.55C21.96,18.13 22.45,16.96 22.03,15.95M7.88,8.75A1,1 0 0,1 6.88,7.75A1,1 0 0,1 7.88,6.75C8.43,6.75 8.88,7.2 8.88,7.75C8.88,8.3 8.43,8.75 7.88,8.75M5.88,19.75A2,2 0 0,0 7.88,21.75H9.33L5.88,13.41V19.75Z","mdi:pencil":"M20.71,7.04C21.1,6.65 21.1,6 20.71,5.63L18.37,3.29C18,2.9 17.35,2.9 16.96,3.29L15.12,5.12L18.87,8.87M3,17.25V21H6.75L17.81,9.93L14.06,6.18L3,17.25Z","mdi:play":"M8,5.14V19.14L19,12.14L8,5.14Z","mdi:play-box-multiple-outline":"M4 6H2V20C2 21.1 2.9 22 4 22H18V20H4V6M20 4V16H8V4H20M20 2H8C6.9 2 6 2.9 6 4V16C6 17.1 6.9 18 8 18H20C21.1 18 22 17.1 22 16V4C22 2.9 21.1 2 20 2M12 14.5V5.5L18 10L12 14.5Z","mdi:plus":"M19,13H13V19H11V13H5V11H11V5H13V11H19V13Z","mdi:plus-circle-multiple-outline":"M16,8H14V11H11V13H14V16H16V13H19V11H16M2,12C2,9.21 3.64,6.8 6,5.68V3.5C2.5,4.76 0,8.09 0,12C0,15.91 2.5,19.24 6,20.5V18.32C3.64,17.2 2,14.79 2,12M15,3C10.04,3 6,7.04 6,12C6,16.96 10.04,21 15,21C19.96,21 24,16.96 24,12C24,7.04 19.96,3 15,3M15,19C11.14,19 8,15.86 8,12C8,8.14 11.14,5 15,5C18.86,5 22,8.14 22,12C22,15.86 18.86,19 15,19Z","mdi:redo":"M18.4,10.6C16.55,9 14.15,8 11.5,8C6.85,8 2.92,11.03 1.54,15.22L3.9,16C4.95,12.81 7.95,10.5 11.5,10.5C13.45,10.5 15.23,11.22 16.62,12.38L13,16H22V7L18.4,10.6Z","mdi:restore":"M13,3A9,9 0 0,0 4,12H1L4.89,15.89L4.96,16.03L9,12H6A7,7 0 0,1 13,5A7,7 0 0,1 20,12A7,7 0 0,1 13,19C11.07,19 9.32,18.21 8.06,16.94L6.64,18.36C8.27,20 10.5,21 13,21A9,9 0 0,0 22,12A9,9 0 0,0 13,3Z","mdi:robot-industrial":"M18.41,4L16,6.41V6.59L18.41,9H22V11H17.59L16,9.41V12H15A2,2 0 0,1 13,10V7.5H9.86C9.77,7.87 9.62,8.22 9.42,8.55L15.18,19H20A2,2 0 0,1 22,21V22H2V21A2,2 0 0,1 4,19H10.61L5.92,10.5C4.12,10.47 2.56,9.24 2.11,7.5C1.56,5.36 2.85,3.18 5,2.63C7.13,2.08 9.31,3.36 9.86,5.5H13V3A2,2 0 0,1 15,1H16V3.59L17.59,2H22V4H18.41M6,4.5A2,2 0 0,0 4,6.5A2,2 0 0,0 6,8.5A2,2 0 0,0 8,6.5A2,2 0 0,0 6,4.5Z","mdi:silverware-fork-knife":"M11,9H9V2H7V9H5V2H3V9C3,11.12 4.66,12.84 6.75,12.97V22H9.25V12.97C11.34,12.84 13,11.12 13,9V2H11V9M16,6V14H18.5V22H21V2C18.24,2 16,4.24 16,6Z","mdi:sine-wave":"M16.5,21C13.5,21 12.31,16.76 11.05,12.28C10.14,9.04 9,5 7.5,5C4.11,5 4,11.93 4,12H2C2,11.63 2.06,3 7.5,3C10.5,3 11.71,7.25 12.97,11.74C13.83,14.8 15,19 16.5,19C19.94,19 20.03,12.07 20.03,12H22.03C22.03,12.37 21.97,21 16.5,21Z","mdi:sofa":"M12.5 7C12.5 5.89 13.39 5 14.5 5H18C19.1 5 20 5.9 20 7V9.16C18.84 9.57 18 10.67 18 11.97V14H12.5V7M6 11.96V14H11.5V7C11.5 5.89 10.61 5 9.5 5H6C4.9 5 4 5.9 4 7V9.15C5.16 9.56 6 10.67 6 11.96M20.66 10.03C19.68 10.19 19 11.12 19 12.12V15H5V12C5 10.9 4.11 10 3 10S1 10.9 1 12V17C1 18.1 1.9 19 3 19V21H5V19H19V21H21V19C22.1 19 23 18.1 23 17V12C23 10.79 21.91 9.82 20.66 10.03Z","mdi:spotlight-beam":"M9,16.5L9.91,15.59L15.13,20.8L14.21,21.71L9,16.5M15.5,10L16.41,9.09L21.63,14.3L20.71,15.21L15.5,10M6.72,2.72L10.15,6.15L6.15,10.15L2.72,6.72C1.94,5.94 1.94,4.67 2.72,3.89L3.89,2.72C4.67,1.94 5.94,1.94 6.72,2.72M14.57,7.5L15.28,8.21L8.21,15.28L7.5,14.57L6.64,11.07L11.07,6.64L14.57,7.5Z","mdi:stop":"M18,18H6V6H18V18Z","mdi:text":"M21,6V8H3V6H21M3,18H12V16H3V18M3,13H21V11H3V13Z","mdi:thermometer":"M15 13V5A3 3 0 0 0 9 5V13A5 5 0 1 0 15 13M12 4A1 1 0 0 1 13 5V8H11V5A1 1 0 0 1 12 4Z","mdi:toggle-switch":"M17,7H7A5,5 0 0,0 2,12A5,5 0 0,0 7,17H17A5,5 0 0,0 22,12A5,5 0 0,0 17,7M17,15A3,3 0 0,1 14,12A3,3 0 0,1 17,9A3,3 0 0,1 20,12A3,3 0 0,1 17,15Z","mdi:tune-vertical":"M7 3H5V9H7V3M19 3H17V13H19V3M3 13H5V21H7V13H9V11H3V13M15 7H13V3H11V7H9V9H15V7M11 21H13V11H11V21M15 15V17H17V21H19V17H21V15H15Z","mdi:tune-vertical-variant":"M8 12.14V2H6V12.14C4.28 12.59 3 14.14 3 16S4.28 19.41 6 19.86V22H8V19.86C9.72 19.41 11 17.86 11 16S9.72 12.59 8 12.14M7 14C8.1 14 9 14.9 9 16S8.1 18 7 18C5.9 18 5 17.1 5 16S5.9 14 7 14M18 2H16V4.14C14.28 4.59 13 6.14 13 8S14.28 11.41 16 11.86V22H18V11.86C19.72 11.41 21 9.86 21 8S19.72 4.59 18 4.14V2M17 6C18.1 6 19 6.9 19 8S18.1 10 17 10C15.9 10 15 9.1 15 8S15.9 6 17 6Z","mdi:undo":"M12.5,8C9.85,8 7.45,9 5.6,10.6L2,7V16H11L7.38,12.38C8.77,11.22 10.54,10.5 12.5,10.5C16.04,10.5 19.05,12.81 20.1,16L22.47,15.22C21.08,11.03 17.15,8 12.5,8Z","mdi:view-dashboard":"M13,3V9H21V3M13,21H21V11H13M3,21H11V15H3M3,13H11V3H3V13Z","mdi:view-grid":"M3,11H11V3H3M3,21H11V13H3M13,21H21V13H13M13,3V11H21V3","mdi:volume-high":"M14,3.23V5.29C16.89,6.15 19,8.83 19,12C19,15.17 16.89,17.84 14,18.7V20.77C18,19.86 21,16.28 21,12C21,7.72 18,4.14 14,3.23M16.5,12C16.5,10.23 15.5,8.71 14,7.97V16C15.5,15.29 16.5,13.76 16.5,12M3,9V15H7L12,20V4L7,9H3Z","mdi:volume-off":"M12,4L9.91,6.09L12,8.18M4.27,3L3,4.27L7.73,9H3V15H7L12,20V13.27L16.25,17.53C15.58,18.04 14.83,18.46 14,18.7V20.77C15.38,20.45 16.63,19.82 17.68,18.96L19.73,21L21,19.73L12,10.73M19,12C19,12.94 18.8,13.82 18.46,14.64L19.97,16.15C20.62,14.91 21,13.5 21,12C21,7.72 18,4.14 14,3.23V5.29C16.89,6.15 19,8.83 19,12M16.5,12C16.5,10.23 15.5,8.71 14,7.97V10.18L16.45,12.63C16.5,12.43 16.5,12.21 16.5,12Z","mdi:water-percent":"M12,3.25C12,3.25 6,10 6,14C6,17.32 8.69,20 12,20A6,6 0 0,0 18,14C18,10 12,3.25 12,3.25M14.47,9.97L15.53,11.03L9.53,17.03L8.47,15.97M9.75,10A1.25,1.25 0 0,1 11,11.25A1.25,1.25 0 0,1 9.75,12.5A1.25,1.25 0 0,1 8.5,11.25A1.25,1.25 0 0,1 9.75,10M14.25,14.5A1.25,1.25 0 0,1 15.5,15.75A1.25,1.25 0 0,1 14.25,17A1.25,1.25 0 0,1 13,15.75A1.25,1.25 0 0,1 14.25,14.5Z","mdi:window-shutter":"M3 4H21V8H19V20H17V8H7V20H5V8H3V4M8 9H16V11H8V9M8 12H16V14H8V12M8 15H16V17H8V15M8 18H16V20H8V18Z"},ff=oe["mdi:check"],gf=oe["mdi:clipboard-text-outline"],mf=oe["mdi:close"],bf=oe["mdi:cog"],vf=oe["mdi:home"],_f=oe["mdi:lan-connect"],yf=oe["mdi:lan-disconnect"],xf=oe["mdi:lightning-bolt"],wf=oe["mdi:magnify"],Ph=oe["mdi:menu"],$f=oe["mdi:menu-open"],Ah=oe["mdi:pencil"],kf=oe["mdi:play-box-multiple-outline"],Cf=oe["mdi:plus"],Sf=oe["mdi:redo"],Ef=oe["mdi:restore"],Pf=oe["mdi:undo"],Ns=oe,ca=Ns["mdi:help-circle"];function Af(e){let t=e.replace(/^mdi/,"");return t=t.replace(/([a-z])([A-Z0-9])/g,"$1-$2"),t=t.replace(/([0-9])([A-Z])/g,"$1-$2"),`mdi:${t.toLowerCase()}`}let Ni=null,Qn=null,Jn=null,Uc=!1;const ds=new Set;function Th(e){return Ni?(e(),()=>{}):(ds.add(e),()=>ds.delete(e))}async function Na(){Ni||(Jn||(Jn=Sr(()=>import("./mdi-icons-CYzTwnk_.js"),[],import.meta.url).then(e=>{Ni=new Map(Object.entries(Ns));const t=[];for(const[i,r]of Object.entries(e)){if(!i.startsWith("mdi")||typeof r!="string")continue;const o=Af(i);Ni.set(o,r),t.push(o)}Qn=t.sort();for(const i of ds)i();ds.clear()})),await Jn)}function Ri(e){if(!e)return ca;if(Ni)return Ni.get(e)??ca;const t=Ns[e];return t||(Tf(),ca)}function Tf(){if(Uc||Ni||Jn)return;Uc=!0;const e=()=>{Na()},t=window.requestIdleCallback;if(typeof t=="function"){t(e,{timeout:1500});return}setTimeout(e,300)}function On(e,t=80){const i=e.trim().toLowerCase();if(!Qn){const o=Object.keys(Ns);return i?o.filter(n=>n.includes(i)).slice(0,t):o.slice(0,t)}if(!i)return Qn.slice(0,t);const r=[];for(const o of Qn)if(o.includes(i)&&(r.push(o),r.length>=t))break;return r}var Mf=Object.defineProperty,Of=Object.getOwnPropertyDescriptor,un=(e,t,i,r)=>{for(var o=r>1?void 0:r?Of(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Mf(t,i,o),o};let hs=class extends y{constructor(){super(...arguments),this.path=""}render(){return h`
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d=${this.path}></path>
      </svg>
    `}};hs.styles=w`
    :host {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: var(--mdc-icon-size, 24px);
      height: var(--mdc-icon-size, 24px);
      line-height: 0;
      flex-shrink: 0;
      vertical-align: middle;
    }
    svg {
      display: block;
      width: 100%;
      height: 100%;
      fill: currentColor;
    }
  `;un([u({attribute:!1})],hs.prototype,"path",2);hs=un([x("ha-svg-icon")],hs);let qo=class extends y{constructor(){super(...arguments),this.icon="",this._revision=0}connectedCallback(){super.connectedCallback(),this._unload=Th(()=>{this._revision++})}disconnectedCallback(){var e;(e=this._unload)==null||e.call(this),super.disconnectedCallback()}render(){this._revision;const e=Ri(this.icon);return h`<ha-svg-icon .path=${e}></ha-svg-icon>`}};qo.styles=w`
    :host {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: var(--mdc-icon-size, 24px);
      height: var(--mdc-icon-size, 24px);
      line-height: 0;
      flex-shrink: 0;
      color: inherit;
    }
  `;un([u()],qo.prototype,"icon",2);un([g()],qo.prototype,"_revision",2);qo=un([x("ha-icon")],qo);var Lf=Object.getOwnPropertyDescriptor,Df=(e,t,i,r)=>{for(var o=r>1?void 0:r?Lf(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=s(o)||o);return o};let Ra=class extends y{render(){return h`<slot></slot>`}};Ra.styles=w`
    :host {
      display: block;
      background: var(--card-background-color, #fff);
      border-radius: var(--ha-card-border-radius, 12px);
      border: var(--ha-card-border-width, 1px) solid var(--divider-color);
      box-shadow: var(--ha-card-box-shadow);
      overflow: var(--ha-card-overflow, hidden);
      height: 100%;
      box-sizing: border-box;
    }
  `;Ra=Df([x("ha-card")],Ra);/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */let Mh=class extends Event{constructor(t,i,r,o){super("context-request",{bubbles:!0,composed:!0}),this.context=t,this.contextTarget=i,this.callback=r,this.subscribe=o??!1}};/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 *//**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */let jc=class{constructor(t,i,r,o){if(this.subscribe=!1,this.provided=!1,this.value=void 0,this.t=(n,s)=>{this.unsubscribe&&(this.unsubscribe!==s&&(this.provided=!1,this.unsubscribe()),this.subscribe||this.unsubscribe()),this.value=n,this.host.requestUpdate(),this.provided&&!this.subscribe||(this.provided=!0,this.callback&&this.callback(n,s)),this.unsubscribe=s},this.host=t,i.context!==void 0){const n=i;this.context=n.context,this.callback=n.callback,this.subscribe=n.subscribe??!1}else this.context=i,this.callback=r,this.subscribe=o??!1;this.host.addController(this)}hostConnected(){this.dispatchRequest()}hostDisconnected(){this.unsubscribe&&(this.unsubscribe(),this.unsubscribe=void 0)}dispatchRequest(){this.host.dispatchEvent(new Mh(this.context,this.host,this.t,this.subscribe))}};/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */let If=class{get value(){return this.o}set value(t){this.setValue(t)}setValue(t,i=!1){const r=i||!Object.is(t,this.o);this.o=t,r&&this.updateObservers()}constructor(t){this.subscriptions=new Map,this.updateObservers=()=>{for(const[i,{disposer:r}]of this.subscriptions)i(this.o,r)},t!==void 0&&(this.value=t)}addCallback(t,i,r){if(!r)return void t(this.value);this.subscriptions.has(t)||this.subscriptions.set(t,{disposer:()=>{this.subscriptions.delete(t)},consumerHost:i});const{disposer:o}=this.subscriptions.get(t);t(this.value,o)}clearCallbacks(){this.subscriptions.clear()}};/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */let Hf=class extends Event{constructor(t,i){super("context-provider",{bubbles:!0,composed:!0}),this.context=t,this.contextTarget=i}},qc=class extends If{constructor(t,i,r){var o,n;super(i.context!==void 0?i.initialValue:r),this.onContextRequest=s=>{if(s.context!==this.context)return;const a=s.contextTarget??s.composedPath()[0];a!==this.host&&(s.stopPropagation(),this.addCallback(s.callback,a,s.subscribe))},this.onProviderRequest=s=>{if(s.context!==this.context||(s.contextTarget??s.composedPath()[0])===this.host)return;const a=new Set;for(const[l,{consumerHost:c}]of this.subscriptions)a.has(l)||(a.add(l),c.dispatchEvent(new Mh(this.context,c,l,!0)));s.stopPropagation()},this.host=t,i.context!==void 0?this.context=i.context:this.context=i,this.attachListeners(),(n=(o=this.host).addController)==null||n.call(o,this)}attachListeners(){this.host.addEventListener("context-request",this.onContextRequest),this.host.addEventListener("context-provider",this.onProviderRequest)}hostConnected(){this.host.dispatchEvent(new Hf(this.context,this.host))}};/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */function zf({context:e}){return(t,i)=>{const r=new WeakMap;if(typeof i=="object")return{get(){return t.get.call(this)},set(o){return r.get(this).setValue(o),t.set.call(this,o)},init(o){return r.set(this,new qc(this,{context:e,initialValue:o})),o}};{t.constructor.addInitializer(s=>{r.set(s,new qc(s,{context:e}))});const o=Object.getOwnPropertyDescriptor(t,i);let n;if(o===void 0){const s=new WeakMap;n={get(){return s.get(this)},set(a){r.get(this).setValue(a),s.set(this,a)},configurable:!0,enumerable:!0}}else{const s=o.set;n={...o,set(a){r.get(this).setValue(a),s==null||s.call(this,a)}}}return void Object.defineProperty(t,i,n)}}}/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */function cr({context:e,subscribe:t}){return(i,r)=>{typeof r=="object"?r.addInitializer(function(){new jc(this,{context:e,callback:o=>{i.set.call(this,o)},subscribe:t})}):i.constructor.addInitializer(o=>{new jc(o,{context:e,callback:n=>{o[r]=n},subscribe:t})})}}/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const wl={ATTRIBUTE:1,CHILD:2},$l=e=>(...t)=>({_$litDirective$:e,values:t});let kl=class{constructor(t){}get _$AU(){return this._$AM._$AU}_$AT(t,i,r){this._$Ct=t,this._$AM=i,this._$Ci=r}_$AS(t,i){return this.update(t,i)}update(t,i){return this.render(...i)}};/**
 * @license
 * Copyright 2018 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const Oh="important",Vf=" !"+Oh,P=$l(class extends kl{constructor(e){var t;if(super(e),e.type!==wl.ATTRIBUTE||e.name!=="style"||((t=e.strings)==null?void 0:t.length)>2)throw Error("The `styleMap` directive must be used in the `style` attribute and must be the only part in the attribute.")}render(e){return Object.keys(e).reduce((t,i)=>{const r=e[i];return r==null?t:t+`${i=i.includes("-")?i:i.replace(/(?:^(webkit|moz|ms|o)|)(?=[A-Z])/g,"-$&").toLowerCase()}:${r};`},"")}update(e,[t]){const{style:i}=e.element;if(this.ft===void 0)return this.ft=new Set(Object.keys(t)),this.render(t);for(const r of this.ft)t[r]==null&&(this.ft.delete(r),r.includes("-")?i.removeProperty(r):i[r]=null);for(const r in t){const o=t[r];if(o!=null){this.ft.add(r);const n=typeof o=="string"&&o.endsWith(Vf);r.includes("-")||n?i.setProperty(r,n?o.slice(0,-11):o,n?Oh:""):i[r]=o}}return Vt}}),vi="flow";function $e(e){if(!e)return!1;const t=e.state;if(typeof t=="boolean")return t;if(typeof t=="number")return t>0;const i=String(t).toLowerCase();return i==="on"||i==="true"||i==="1"||i==="open"||i==="playing"}function Cl(e){const t=$e(e);return typeof(e==null?void 0:e.state)=="number"?t?0:100:!t}const Oo=[{value:"primary",label:"主题色"},{value:"accent",label:"强调色"},{value:"red",label:"红色"},{value:"pink",label:"粉色"},{value:"purple",label:"紫色"},{value:"deep-purple",label:"深紫"},{value:"indigo",label:"靛蓝"},{value:"blue",label:"蓝色"},{value:"light-blue",label:"浅蓝"},{value:"cyan",label:"青色"},{value:"teal",label:"蓝绿"},{value:"green",label:"绿色"},{value:"light-green",label:"浅绿"},{value:"lime",label:"青柠"},{value:"yellow",label:"黄色"},{value:"amber",label:"琥珀色"},{value:"orange",label:"橙色"},{value:"deep-orange",label:"深橙"},{value:"brown",label:"棕色"},{value:"grey",label:"灰色"},{value:"blue-grey",label:"蓝灰"}],Nf=new Set(Oo.map(e=>e.value));function Nt(e){return Nf.has(e)?`var(--${e}-color, var(--primary-color))`:e}function Rf(e){var t;return((t=Oo.find(i=>i.value===e))==null?void 0:t.label)??e}const qi=26,Fa=[{value:22,label:"小"},{value:26,label:"中"},{value:32,label:"大"},{value:40,label:"更大"}];function Sl(e){return(e==null?void 0:e.show_icon)!==!1}function Rt(e){return e?typeof e.show_name=="boolean"?e.show_name:typeof e.show_state=="boolean"?e.show_state:!0:!0}function di(e){return(e==null?void 0:e.show_background)!==!1}function fn(e,t=qi){const i=e==null?void 0:e.size;let r=typeof i=="number"?i:Number(i);Number.isFinite(r)&&r>48&&(r=Math.round(r*26/56));const o=Number.isFinite(r)&&r>0?r:t,n=Number.isFinite(t)&&t>0?t:qi;return Math.min(80,Math.max(18,Number.isFinite(o)&&o>0?o:n))}function Ff(e){var r;let t=((r=Fa[1])==null?void 0:r.value)??qi,i=Number.POSITIVE_INFINITY;for(const o of Fa){const n=Math.abs(o.value-e);n<i&&(t=o.value,i=n)}return t}var Bf=Object.defineProperty,Uf=Object.getOwnPropertyDescriptor,Lh=(e,t,i,r)=>{for(var o=r>1?void 0:r?Uf(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Bf(t,i,o),o};let ps=class extends y{constructor(){super(...arguments),this.type="text"}render(){return h`
      <div
        class="heading-badge"
        role=${this.type==="button"?"button":"group"}
        tabindex=${this.type==="button"?"0":"-1"}
      >
        <slot name="icon"></slot>
        <slot></slot>
      </div>
    `}};ps.styles=w`
    .heading-badge {
      display: inline-flex;
      flex-direction: row;
      align-items: center;
      gap: 4px;
      white-space: nowrap;
      color: var(--ha-heading-badge-text-color, var(--secondary-text-color));
      font-size: var(--ha-heading-badge-font-size, 14px);
      font-weight: 400;
      line-height: 20px;
      letter-spacing: 0.1px;
    }
    ::slotted([slot="icon"]) {
      display: flex;
      color: var(--icon-color, inherit);
      --mdc-icon-size: 16px;
    }
    :host([type="button"]) .heading-badge,
    .heading-badge[role="button"] {
      cursor: pointer;
    }
  `;Lh([u()],ps.prototype,"type",2);ps=Lh([x("ha-heading-badge")],ps);var jf=Object.defineProperty,qf=Object.getOwnPropertyDescriptor,gn=(e,t,i,r)=>{for(var o=r>1?void 0:r?qf(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&jf(t,i,o),o};let Gi=class extends y{constructor(){super(...arguments),this.statesRevision=0,this._tick=0}connectedCallback(){super.connectedCallback(),this._syncSubscription()}disconnectedCallback(){var e;(e=this._unsub)==null||e.call(this),this._unsub=void 0,this._subscribedEntity=void 0,super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("config"))&&this._syncSubscription()}_entityId(){var e;return(e=this.config)!=null&&e.entity?String(this.config.entity).trim():""}_syncSubscription(){var t;const e=this._entityId();e===this._subscribedEntity&&this._unsub||((t=this._unsub)==null||t.call(this),this._unsub=void 0,this._subscribedEntity=void 0,!(!e||!this.flow)&&(this._subscribedEntity=e,this._unsub=this.flow.subscribeEntity(e,()=>{this._tick+=1})))}_stateText(){const e=this._entityId();return!e||!this.flow?"—":this.flow.formatState(e)}_iconColor(){var o;const e=(o=this.config)!=null&&o.color?String(this.config.color).trim():"";if(!e||e==="none")return;const t=this._entityId(),i=t&&this.flow?this.flow.states[t]:void 0,r=$e(i);return e==="state"?r?"var(--state-icon-color)":void 0:r?Nt(e):void 0}render(){var l;const e=this.config;if(!e)return m;this._tick,this.statesRevision;const t=this._entityId(),i=t&&this.flow?this.flow.states[t]:void 0,r=Sl(e),o=Rt(e),n=!t,s=n?"mdi:alert-circle":e.icon?String(e.icon):(l=i==null?void 0:i.attributes)!=null&&l.icon?String(i.attributes.icon):"mdi:eye",a=n?"var(--error-color)":this._iconColor();return h`
      <ha-heading-badge
        class=${n?"error":""}
        style=${P({"--icon-color":a})}
      >
        ${r?h`<ha-icon slot="icon" .icon=${s}></ha-icon>`:m}
        ${o?h`<span>${n?"—":this._stateText()}</span>`:m}
      </ha-heading-badge>
    `}};Gi.styles=w`
    :host {
      display: inline-flex;
    }
    ha-heading-badge.error {
      --icon-color: var(--error-color, #b91c1c);
    }
  `;gn([cr({context:vi,subscribe:!0}),u({attribute:!1})],Gi.prototype,"flow",2);gn([u({attribute:!1})],Gi.prototype,"config",2);gn([u({type:Number,attribute:!1})],Gi.prototype,"statesRevision",2);gn([g()],Gi.prototype,"_tick",2);Gi=gn([x("hui-entity-heading-badge")],Gi);/**
 * @license
 * Copyright 2018 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const T=$l(class extends kl{constructor(e){var t;if(super(e),e.type!==wl.ATTRIBUTE||e.name!=="class"||((t=e.strings)==null?void 0:t.length)>2)throw Error("`classMap()` can only be used in the `class` attribute and must be the only part in the attribute.")}render(e){return" "+Object.keys(e).filter(t=>e[t]).join(" ")+" "}update(e,[t]){var r,o;if(this.st===void 0){this.st=new Set,e.strings!==void 0&&(this.nt=new Set(e.strings.join(" ").split(/\s/).filter(n=>n!=="")));for(const n in t)t[n]&&!((r=this.nt)!=null&&r.has(n))&&this.st.add(n);return this.render(t)}const i=e.element.classList;for(const n of this.st)n in t||(i.remove(n),this.st.delete(n));for(const n in t){const s=!!t[n];s===this.st.has(n)||(o=this.nt)!=null&&o.has(n)||(s?(i.add(n),this.st.add(n)):(i.remove(n),this.st.delete(n)))}return Vt}});function dr(e,t){if(!e)return;const i=t?String(t).trim():"";if(i)return Nt(i)}function hr(e,t,i){const r=dr(e,t);if(r)return r;if(e)return i?Nt(i):void 0}function Gf(e,t){return hr(e,t,"amber")}function Xf(e,t){return dr(e,t)}function El(e){const t=e?String(e).trim():"";if(t)return Nt(t)}let wo=null;function Wf(e){return new Promise(t=>{wo=t,document.dispatchEvent(new CustomEvent("flow-show-badge-card",{bubbles:!0,composed:!0,detail:e}))})}function Yf(){wo==null||wo(),wo=null}function Dh(e){return e.tap_action.press_value===void 0?!0:Dr(e.tap_action.press_value)}function Ih(e){return e.tap_action.release_value===void 0?!1:Dr(e.tap_action.release_value)}function Pl(e,t){const i=Ge(e),r=i.tap_action.action;if(r==="switch")return $e(t);if(r==="rw"){if(!t)return;const o=t.state,n=Dh(i),s=Ih(i);return Bc(o,n)?!0:Bc(o,s)?!1:void 0}}function Ba(e,t,i){if(typeof i=="boolean")return i;const r=Pl(e,t);return typeof r=="boolean"?r:!1}function Zf(e){const t=e.tap_action.card;if(!t||typeof t!="object")return;const i=String(e.entity??"").trim(),r=String(t.entity??"").trim()||i;if(String(t.type??"").trim())return{...t,type:String(t.type).trim(),...r?{entity:r}:{},...e.name&&!t.name?{name:e.name}:{},...e.icon&&!t.icon?{icon:e.icon}:{}}}async function Hh(e,t,i={}){if(!e||!t||i.preview)return{mode:"none"};const r=Ge(t);if(r.type!=="button")return{mode:"none"};const o=r.tap_action.action;if(o==="none")return{mode:"none"};if(o==="open-card"){const s=Zf(r);if(!s)return{mode:"none"};const a=String(r.text??r.name??s.name??"").trim();return Wf({flow:e,cardConfig:s,title:a}),{mode:"none"}}const n=String(r.entity??"").trim();if(!n)return{mode:"none"};if(o==="switch"){const s=e.states[n],l=!Ba(t,s,i.latched);return await e.callService(n,l),{mode:"latch",pressed:l}}if(o==="rw"){const s=e.states[n],l=!Ba(t,s,i.latched);return await e.callService(n,l?Dh(r):Ih(r)),{mode:"latch",pressed:l}}return await e.callService(n,!0),{mode:"pulse"}}function zh(e,t,i={}){const r=Ge(e);return r.type!=="button"?!1:xl(r.tap_action.action)?Ba(e,t,i.latched):!!i.flashing}function Vh(e,t){var r;if(t)return!1;const i=Ge(e);return i.type!=="button"?!1:i.tap_action.action==="open-card"?!!((r=i.tap_action.card)!=null&&r.type):!!String(i.entity??"").trim()}var Kf=Object.defineProperty,Qf=Object.getOwnPropertyDescriptor,pr=(e,t,i,r)=>{for(var o=r>1?void 0:r?Qf(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Kf(t,i,o),o};let Ft=class extends y{constructor(){super(...arguments),this.statesRevision=0,this._flashing=!1,this._tick=0}connectedCallback(){super.connectedCallback(),this._syncSubscription()}disconnectedCallback(){var e;(e=this._unsub)==null||e.call(this),this._unsub=void 0,this._subscribedEntity=void 0,this._pulseTimer&&clearTimeout(this._pulseTimer),super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("config"))&&this._syncSubscription(),this._syncLatchedFromState()}_entityId(){var e;return(e=this.config)!=null&&e.entity?String(this.config.entity).trim():""}_entityState(){const e=this._entityId();if(!(!e||!this.flow))return this.flow.states[e]}_syncSubscription(){var t;const e=this._entityId();e===this._subscribedEntity&&this._unsub||((t=this._unsub)==null||t.call(this),this._unsub=void 0,this._subscribedEntity=void 0,!(!e||!this.flow)&&(this._subscribedEntity=e,this._unsub=this.flow.subscribeEntity(e,()=>{this._syncLatchedFromState(),this._tick+=1})))}_syncLatchedFromState(){if(typeof this._latched!="boolean")return;Pl(this.config,this._entityState())===this._latched&&(this._latched=void 0)}_pulse(){this._flashing=!0,this._pulseTimer&&clearTimeout(this._pulseTimer),this._pulseTimer=setTimeout(()=>{this._flashing=!1,this._pulseTimer=void 0},700)}async _tap(e){e.stopPropagation();const t=await Hh(this.flow,this.config,{latched:this._latched});if(t.mode==="pulse"){this._pulse();return}t.mode==="latch"&&(this._latched=t.pressed)}_iconColor(e){var i;return e||$e(this._entityState())?dr(!0,(i=this.config)==null?void 0:i.color)??"var(--state-icon-color, var(--primary-color))":"var(--state-inactive-color, var(--secondary-text-color))"}render(){const e=this.config;if(!e)return m;this._tick,this.statesRevision;const t=Ge(e),i=t.icon??"",r=t.text||t.name||"",o=Rt(e)&&!!r,n=di(e),s=fn(e,qi),a=Math.max(12,Math.round(s*16/26)),l=Math.max(11,Math.round(s*12/26)),c=this._entityState(),d=xl(t.tap_action.action),p=zh(e,c,{flashing:this._flashing,latched:this._latched}),f=this._iconColor(p),b=Vh(e);return h`
      <button
        type="button"
        class=${T({badge:!0,"with-text":o,transparent:!n,flashing:!d&&this._flashing,pressed:d&&p})}
        style=${P({"--color":f,"--badge-height":`${s}px`,"--badge-icon-size":`${a}px`,"--badge-font-size":`${l}px`})}
        ?disabled=${!b}
        aria-pressed=${d?p?"true":"false":"undefined"}
        @click=${this._tap}
      >
        ${i?h`<ha-icon .icon=${i}></ha-icon>`:m}
        ${o?h`<span class="text">${r}</span>`:m}
      </button>
    `}};Ft.styles=w`
    :host {
      display: inline-flex;
    }
    .badge {
      display: inline-flex;
      flex-direction: row;
      align-items: center;
      justify-content: center;
      gap: 2px;
      height: var(--badge-height, 26px);
      min-width: var(--badge-height, 26px);
      padding: 0;
      border: none;
      border-radius: 999px;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--color, var(--secondary-text-color));
      cursor: pointer;
      font: inherit;
      font-size: var(--badge-font-size, 12px);
      font-weight: 500;
      line-height: 1;
      white-space: nowrap;
      user-select: none;
    }
    .badge.with-text {
      padding: 0 8px;
    }
    .badge.transparent {
      background: transparent;
    }
    .badge:not(.transparent).flashing,
    .badge:not(.transparent).pressed {
      background: color-mix(in srgb, var(--color, var(--primary-color)) 22%, transparent);
    }
    .badge:disabled {
      cursor: default;
      opacity: 0.55;
    }
    .badge:not(:disabled):hover {
      filter: brightness(0.96);
    }
    .badge.transparent:not(:disabled):hover {
      filter: none;
      opacity: 0.85;
    }
    ha-icon {
      --mdc-icon-size: var(--badge-icon-size, 16px);
    }
    .text {
      padding: 0 4px;
    }
  `;pr([u({attribute:!1})],Ft.prototype,"flow",2);pr([u({attribute:!1})],Ft.prototype,"config",2);pr([u({type:Number,attribute:!1})],Ft.prototype,"statesRevision",2);pr([g()],Ft.prototype,"_flashing",2);pr([g()],Ft.prototype,"_latched",2);pr([g()],Ft.prototype,"_tick",2);Ft=pr([x("hui-button-heading-badge")],Ft);var Jf=Object.defineProperty,eg=Object.getOwnPropertyDescriptor,Al=(e,t,i,r)=>{for(var o=r>1?void 0:r?eg(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Jf(t,i,o),o};let Ir=class extends y{constructor(){super(...arguments),this._badgeUnsubs=[],this._subscribedKey=""}static getStubConfig(){return{...wh}}setConfig(e){const t=Array.isArray(e.badges)?e.badges.map(i=>bi(Ge(i))):e.badges;this._config=t?{...e,badges:t}:e,this._syncBadgeSubscriptions()}connectedCallback(){super.connectedCallback(),this._syncBadgeSubscriptions()}disconnectedCallback(){this._clearBadgeSubscriptions(),super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("_config"))&&this._syncBadgeSubscriptions()}_clearBadgeSubscriptions(){this._badgeUnsubs.forEach(e=>e()),this._badgeUnsubs=[],this._subscribedKey=""}_syncBadgeSubscriptions(){var r;const e=this.flow,t=((r=this._config)==null?void 0:r.badges)??[],i=`${e?"1":"0"}:${t.map(o=>String(o.entity??"").trim()).join("\0")}`;i===this._subscribedKey&&this._badgeUnsubs.length>0||(this._clearBadgeSubscriptions(),e&&(this._subscribedKey=i,t.forEach(o=>{const n=o.entity?String(o.entity).trim():"";n&&this._badgeUnsubs.push(e.subscribeEntity(n,()=>{this.requestUpdate()}))})))}getGridOptions(){return{columns:"full",rows:"auto",min_columns:3}}_renderBadge(e){var t,i;return oi(e)?h`
        <hui-button-heading-badge
          .flow=${this.flow}
          .config=${e}
          .statesRevision=${((t=this.flow)==null?void 0:t.statesRevision)??0}
        ></hui-button-heading-badge>
      `:h`
        <hui-entity-heading-badge
          .flow=${this.flow}
          .config=${e}
          .statesRevision=${((i=this.flow)==null?void 0:i.statesRevision)??0}
        ></hui-entity-heading-badge>
    `}render(){var o;if(!this._config)return m;(o=this.flow)==null||o.statesRevision;const e=String(this._config.heading??""),t=this._config.icon?String(this._config.icon):void 0,i=String(this._config.heading_style??"title"),r=this._config.badges??[];return h`
      <ha-card>
        <div class="container">
          <div class="content ${i}">
            ${t?h`<ha-icon .icon=${t}></ha-icon>`:m}
            ${e?h`<p>${e}</p>`:m}
          </div>
          ${r.length?h`
                <div class="badges">
                  <div class="badges-row">
                    ${r.map(n=>this._renderBadge(n))}
                  </div>
                </div>
              `:m}
        </div>
      </ha-card>
    `}};Ir.styles=w`
    :host {
      display: block;
      width: 100%;
      height: 100%;
    }
    ha-card {
      background: none;
      border: none;
      box-shadow: none;
      backdrop-filter: none;
      padding: 0;
      display: flex;
      flex-direction: column;
      justify-content: flex-end;
      height: 100%;
      min-height: 24px;
      --ha-card-overflow: visible;
    }
    .container {
      padding: 0 var(--ha-space-1, 4px);
      display: flex;
      flex-direction: row;
      justify-content: space-between;
      flex-wrap: nowrap;
      align-items: center;
      overflow: visible;
      gap: var(--ha-space-2, 8px);
      min-height: 28px;
      width: 100%;
      box-sizing: border-box;
    }
    .content {
      display: flex;
      flex-direction: row;
      align-items: center;
      gap: var(--ha-space-2, 8px);
      min-width: 0;
      flex: 0 1 max-content;
      color: var(--ha-heading-card-title-color, var(--primary-text-color));
      font-size: var(--ha-heading-card-title-font-size, 18px);
      font-weight: var(--ha-heading-card-title-font-weight, 400);
      line-height: var(--ha-line-height-normal, 1.4);
      letter-spacing: 0.1px;
      --mdc-icon-size: 18px;
    }
    .container .content:not(:only-child) {
      flex: 1 0 var(--ha-heading-card-title-min-width, 150px);
      max-width: max-content;
    }
    .content.subtitle {
      color: var(--ha-heading-card-subtitle-color, var(--secondary-text-color));
      font-size: var(--ha-heading-card-subtitle-font-size, 14px);
      font-weight: var(--ha-font-weight-medium, 500);
      line-height: var(--ha-line-height-condensed, 1.25);
    }
    .content p {
      margin: 0;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
      flex-shrink: 1;
      min-width: 0;
    }
    .badges {
      position: relative;
      display: flex;
      flex: 1 1 auto;
      min-width: 0;
      overflow: auto;
      max-width: 100%;
      scrollbar-width: none;
      justify-content: flex-end;
    }
    .badges-row {
      display: flex;
      flex-direction: row;
      align-items: center;
      flex-wrap: nowrap;
      justify-content: flex-end;
      gap: var(--ha-space-2, 8px);
    }
    .badges-row > * {
      min-width: fit-content;
    }
  `;Al([u({attribute:!1})],Ir.prototype,"flow",2);Al([g()],Ir.prototype,"_config",2);Ir=Al([x("hui-heading-card")],Ir);var tg=Object.defineProperty,ig=Object.getOwnPropertyDescriptor,Nh=(e,t,i,r)=>{for(var o=r>1?void 0:r?ig(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&tg(t,i,o),o};let Go=class extends y{setConfig(e){this._config=e}getGridOptions(){return{columns:"full",rows:2}}render(){var t;const e=String(((t=this._config)==null?void 0:t.message)??"Unknown error");return h`
      <ha-card>
        <div class="error">${e}</div>
      </ha-card>
    `}};Go.styles=w`
    .error {
      padding: 16px;
      color: var(--error-color, #b91c1c);
      font-size: 14px;
    }
  `;Nh([u({attribute:!1})],Go.prototype,"flow",2);Go=Nh([x("hui-error-card")],Go);function Tl(){return{async:!1,breaks:!1,extensions:null,gfm:!0,hooks:null,pedantic:!1,renderer:null,silent:!1,tokenizer:null,walkTokens:null}}var ur=Tl();function Rh(e){ur=e}var Ii={exec:()=>null};function yr(e){let t=[];return i=>{let r=Math.max(0,Math.min(3,i-1)),o=t[r];return o||(o=e(r),t[r]=o),o}}function M(e,t=""){let i=typeof e=="string"?e:e.source,r={replace:(o,n)=>{let s=typeof n=="string"?n:n.source;return s=s.replace(le.caret,"$1"),i=i.replace(o,s),r},getRegex:()=>new RegExp(i,t)};return r}var rg=((e="")=>{try{return!!new RegExp("(?<=1)(?<!1)"+e)}catch{return!1}})(),le={codeRemoveIndent:/^(?: {1,4}| {0,3}\t)/gm,outputLinkReplace:/\\([\[\]])/g,indentCodeCompensation:/^(\s+)(?:```)/,beginningSpace:/^\s+/,endingHash:/#$/,startingSpaceChar:/^ /,endingSpaceChar:/ $/,nonSpaceChar:/[^ ]/,newLineCharGlobal:/\n/g,tabCharGlobal:/\t/g,multipleSpaceGlobal:/\s+/g,blankLine:/^[ \t]*$/,doubleBlankLine:/\n[ \t]*\n[ \t]*$/,blockquoteStart:/^ {0,3}>/,blockquoteSetextReplace:/\n {0,3}((?:=+|-+) *)(?=\n|$)/g,blockquoteSetextReplace2:/^ {0,3}>[ \t]?/gm,listReplaceNesting:/^ {1,4}(?=( {4})*[^ ])/g,listIsTask:/^\[[ xX]\] +\S/,listReplaceTask:/^\[[ xX]\] +/,listTaskCheckbox:/\[[ xX]\]/,anyLine:/\n.*\n/,hrefBrackets:/^<(.*)>$/,tableDelimiter:/[:|]/,tableAlignChars:/^\||\| *$/g,tableRowBlankLine:/\n[ \t]*$/,tableAlignRight:/^ *-+: *$/,tableAlignCenter:/^ *:-+: *$/,tableAlignLeft:/^ *:-+ *$/,startATag:/^<a /i,endATag:/^<\/a>/i,startPreScriptTag:/^<(pre|code|kbd|script)(\s|>)/i,endPreScriptTag:/^<\/(pre|code|kbd|script)(\s|>)/i,startAngleBracket:/^</,endAngleBracket:/>$/,pedanticHrefTitle:/^([^'"]*[^\s])\s+(['"])(.*)\2/,unicodeAlphaNumeric:/[\p{L}\p{N}]/u,escapeTest:/[&<>"']/,escapeReplace:/[&<>"']/g,escapeTestNoEncode:/[<>"']|&(?!(#\d{1,7}|#[Xx][a-fA-F0-9]{1,6}|\w+);)/,escapeReplaceNoEncode:/[<>"']|&(?!(#\d{1,7}|#[Xx][a-fA-F0-9]{1,6}|\w+);)/g,caret:/(^|[^\[])\^/g,percentDecode:/%25/g,findPipe:/\|/g,splitPipe:/ \|/,slashPipe:/\\\|/g,carriageReturn:/\r\n|\r/g,spaceLine:/^ +$/gm,notSpaceStart:/^\S*/,endingNewline:/\n$/,listItemRegex:e=>new RegExp(`^( {0,3}${e})((?:[	 ][^\\n]*)?(?:\\n|$))`),nextBulletRegex:yr(e=>new RegExp(`^ {0,${e}}(?:[*+-]|\\d{1,9}[.)])((?:[ 	][^\\n]*)?(?:\\n|$))`)),hrRegex:yr(e=>new RegExp(`^ {0,${e}}((?:- *){3,}|(?:_ *){3,}|(?:\\* *){3,})(?:\\n+|$)`)),fencesBeginRegex:yr(e=>new RegExp(`^ {0,${e}}(?:\`\`\`|~~~)`)),headingBeginRegex:yr(e=>new RegExp(`^ {0,${e}}#`)),htmlBeginRegex:yr(e=>new RegExp(`^ {0,${e}}<(?:[a-z].*>|!--)`,"i")),blockquoteBeginRegex:yr(e=>new RegExp(`^ {0,${e}}>`))},og=/^(?:[ \t]*(?:\n|$))+/,ng=/^((?: {4}| {0,3}\t)[^\n]+(?:\n(?:[ \t]*(?:\n|$))*)?)+/,sg=/^ {0,3}(`{3,}(?=[^`\n]*(?:\n|$))|~{3,})([^\n]*)(?:\n|$)(?:|([\s\S]*?)(?:\n|$))(?: {0,3}\1[~`]* *(?=\n|$)|$)/,mn=/^ {0,3}((?:-[\t ]*){3,}|(?:_[ \t]*){3,}|(?:\*[ \t]*){3,})(?:\n+|$)/,ag=/^ {0,3}(#{1,6})(?=\s|$)(.*)(?:\n+|$)/,Ml=/ {0,3}(?:[*+-]|\d{1,9}[.)])/,Fh=/^(?!bull |blockCode|fences|blockquote|heading|html|table)((?:.|\n(?!\s*?\n|bull |blockCode|fences|blockquote|heading|html|table))+?)\n {0,3}(=+|-+) *(?:\n+|$)/,Bh=M(Fh).replace(/bull/g,Ml).replace(/blockCode/g,/(?: {4}| {0,3}\t)/).replace(/fences/g,/ {0,3}(?:`{3,}|~{3,})/).replace(/blockquote/g,/ {0,3}>/).replace(/heading/g,/ {0,3}#{1,6}(?:\s|$)/).replace(/html/g,/ {0,3}<[^\n>]+>\n/).replace(/\|table/g,"").getRegex(),lg=M(Fh).replace(/bull/g,Ml).replace(/blockCode/g,/(?: {4}| {0,3}\t)/).replace(/fences/g,/ {0,3}(?:`{3,}|~{3,})/).replace(/blockquote/g,/ {0,3}>/).replace(/heading/g,/ {0,3}#{1,6}(?:\s|$)/).replace(/html/g,/ {0,3}<[^\n>]+>\n/).replace(/table/g,/ {0,3}\|?(?:[:\- ]*\|)+[\:\- ]*\n/).getRegex(),Ol=/^([^\n]+(?:\n(?!hr|heading|lheading|blockquote|fences|list|html|table|[ \t]+\n)[^\n]+)*)/,cg=/^[^\n]+/,Ll=/(?!\s*\])(?:\\[\s\S]|[^\[\]\\])+/,dg=M(/^ {0,3}\[(label)\]: *(?:\n[ \t]*)?([^<\s][^\s]*|<.*?>)(?:(?: +(?:\n[ \t]*)?| *\n[ \t]*)(title))? *(?:\n+|$)/).replace("label",Ll).replace("title",/(?:"(?:\\"?|[^"\\])*"|'[^'\n]*(?:\n[^'\n]+)*\n?'|\([^()]*\))/).getRegex(),hg=M(/^(bull)([ \t][^\n]*?)?(?:\n|$)/).replace(/bull/g,Ml).getRegex(),Rs="address|article|aside|base|basefont|blockquote|body|caption|center|col|colgroup|dd|details|dialog|dir|div|dl|dt|fieldset|figcaption|figure|footer|form|frame|frameset|h[1-6]|head|header|hr|html|iframe|legend|li|link|main|menu|menuitem|meta|nav|noframes|ol|optgroup|option|p|param|search|section|summary|table|tbody|td|tfoot|th|thead|title|tr|track|ul",Dl=/<!--(?:-?>|[\s\S]*?(?:-->|$))/,pg=M("^ {0,3}(?:<(script|pre|style|textarea)[\\s>][\\s\\S]*?(?:</\\1>[^\\n]*\\n*|$)|comment[^\\n]*(\\n+|$)|<\\?[\\s\\S]*?(?:\\?>[^\\n]*\\n*|$)|<![A-Z][\\s\\S]*?(?:>[^\\n]*\\n*|$)|<!\\[CDATA\\[[\\s\\S]*?(?:\\]\\]>[^\\n]*\\n*|$)|</?(tag)(?: +|\\n|/?>)[\\s\\S]*?(?:(?:\\n[ 	]*)+\\n|$)|<(?!script|pre|style|textarea)([a-z][\\w-]*)(?:attribute)*? */?>(?=[ \\t]*(?:\\n|$))[\\s\\S]*?(?:(?:\\n[ 	]*)+\\n|$)|</(?!script|pre|style|textarea)[a-z][\\w-]*\\s*>(?=[ \\t]*(?:\\n|$))[\\s\\S]*?(?:(?:\\n[ 	]*)+\\n|$))","i").replace("comment",Dl).replace("tag",Rs).replace("attribute",/ +[a-zA-Z:_][\w.:-]*(?: *= *"[^"\n]*"| *= *'[^'\n]*'| *= *[^\s"'=<>`]+)?/).getRegex(),Uh=e=>M(Ol).replace("hr",mn).replace("heading"," {0,3}#{1,6}(?:\\s|$)").replace("|lheading","").replace("|table","").replace("blockquote"," {0,3}>").replace("fences"," {0,3}(?:`{3,}(?=[^`\\n]*(?:\\n|$))|~~~)[^\\n]*(?:\\n|$)").replace("list",e).replace("html","</?(?:tag)(?: +|\\n|/?>)|<(?:script|pre|style|textarea|!--)").replace("tag",Rs).getRegex(),ug=Uh(/ {0,3}(?:[*+-]|1[.)])[ \t]+[^ \t\n]/),fg=Uh(/ {0,3}(?:[*+-]|\d{1,9}[.)])(?:[ \t]|\n|$)/),gg=M(/^( {0,3}> ?(paragraph|[^\n]*)(?:\n|$))+/).replace("paragraph",fg).getRegex(),Il={blockquote:gg,code:ng,def:dg,fences:sg,heading:ag,hr:mn,html:pg,lheading:Bh,list:hg,newline:og,paragraph:ug,table:Ii,text:cg},Gc=M("^ *([^\\n ].*)\\n {0,3}((?:\\| *)?:?-+:? *(?:\\| *:?-+:? *)*(?:\\| *)?)(?:\\n((?:(?! *\\n|hr|heading|blockquote|code|fences|list|html).*(?:\\n|$))*)\\n*|$)").replace("hr",mn).replace("heading"," {0,3}#{1,6}(?:\\s|$)").replace("blockquote"," {0,3}>").replace("code","(?: {4}| {0,3}	)[^\\n]").replace("fences"," {0,3}(?:`{3,}(?=[^`\\n]*(?:\\n|$))|~~~)[^\\n]*(?:\\n|$)").replace("list"," {0,3}(?:[*+-]|1[.)])[ \\t]").replace("html","</?(?:tag)(?: +|\\n|/?>)|<(?:script|pre|style|textarea|!--)").replace("tag",Rs).getRegex(),mg={...Il,lheading:lg,table:Gc,paragraph:M(Ol).replace("hr",mn).replace("heading"," {0,3}#{1,6}(?:\\s|$)").replace("|lheading","").replace("table",Gc).replace("blockquote"," {0,3}>").replace("fences"," {0,3}(?:`{3,}(?=[^`\\n]*(?:\\n|$))|~~~)[^\\n]*(?:\\n|$)").replace("list"," {0,3}(?:[*+-]|1[.)])[ \\t]+[^ \\t\\n]").replace("html","</?(?:tag)(?: +|\\n|/?>)|<(?:script|pre|style|textarea|!--)").replace("tag",Rs).getRegex()},bg={...Il,html:M(`^ *(?:comment *(?:\\n|\\s*$)|<(tag)[\\s\\S]+?</\\1> *(?:\\n{2,}|\\s*$)|<tag(?:"[^"]*"|'[^']*'|\\s[^'"/>\\s]*)*?/?> *(?:\\n{2,}|\\s*$))`).replace("comment",Dl).replace(/tag/g,"(?!(?:a|em|strong|small|s|cite|q|dfn|abbr|data|time|code|var|samp|kbd|sub|sup|i|b|u|mark|ruby|rt|rp|bdi|bdo|span|br|wbr|ins|del|img)\\b)\\w+(?!:|[^\\w\\s@]*@)\\b").getRegex(),def:/^ *\[([^\]]+)\]: *<?([^\s>]+)>?(?: +(["(][^\n]+[")]))? *(?:\n+|$)/,heading:/^(#{1,6})(.*)(?:\n+|$)/,fences:Ii,lheading:/^(.+?)\n {0,3}(=+|-+) *(?:\n+|$)/,paragraph:M(Ol).replace("hr",mn).replace("heading",` *#{1,6} *[^
]`).replace("lheading",Bh).replace("|table","").replace("blockquote"," {0,3}>").replace("|fences","").replace("|list","").replace("|html","").replace("|tag","").getRegex()},vg=/^\\([!"#$%&'()*+,\-./:;<=>?@\[\]\\^_`{|}~])/,_g=/^(`+)([^`]|[^`][\s\S]*?[^`])\1(?!`)/,jh=/^( {2,}|\\)\n(?!\s*$)/,yg=/^(`+|[^`])(?:(?= {2,}\n)|[\s\S]*?(?:(?=[\\<!\[`*_]|\b_|$)|[^ ](?= {2,}\n)))/,Gt=/[\p{P}\p{S}]/u,no=/[\s\p{P}\p{S}]/u,bn=/[^\s\p{P}\p{S}]/u,xg=M(/^((?![*_])punctSpace)/,"u").replace(/punctSpace/g,no).getRegex(),wg=/[\p{Pi}\p{Ps}"']/u,qh=/(?!~)[\p{P}\p{S}]/u,$g=/(?!~)[\s\p{P}\p{S}]/u,kg=/(?:[^\s\p{P}\p{S}]|~)/u,Cg=M(/link|precode-code|html/,"g").replace("link",/\[(?:[^\[\]`]|(?<a>`+)[^`]+\k<a>(?!`))*?\]\((?:\\[\s\S]|[^\\\(\)]|\((?:\\[\s\S]|[^\\\(\)])*\))*\)/).replace("precode-",rg?"(?<!`)()":"(^^|[^`])").replace("code",/(?<b>`+)[^`]+\k<b>(?!`)/).replace("html",/<(?! )[^<>]*?>/).getRegex(),Gh=/^(?:\*+(?:((?!\*)punct)|([^\s*]))?)|^_+(?:((?!_)punct)|([^\s_]))?/,Sg=M(Gh,"u").replace(/punct/g,Gt).getRegex(),Eg=M(Gh,"u").replace(/punct/g,qh).getRegex(),Pg=/^(?:\*+(?:((?!\*)(?!openQuote)punct)|([^\s*]))?)|^_+(?:((?!_)(?!openQuote)punct)|([^\s_]))?/,Ag=M(Pg,"u").replace(/openQuote/g,wg).replace(/punct/g,Gt).getRegex(),Xh="^[^_*]*?__[^_*]*?\\*[^_*]*?(?=__)|[^*]+(?=[^*])|(?!\\*)punct(\\*+)(?=[\\s]|$)|notPunctSpace(\\*+)(?!\\*)(?=punctSpace|$)|(?!\\*)punctSpace(\\*+)(?=notPunctSpace)|[\\s](\\*+)(?!\\*)(?=punct)|(?!\\*)punct(\\*+)(?!\\*)(?=punct)|notPunctSpace(\\*+)(?=notPunctSpace)",Tg=M(Xh,"gu").replace(/notPunctSpace/g,bn).replace(/punctSpace/g,no).replace(/punct/g,Gt).getRegex(),Mg=M(Xh,"gu").replace(/notPunctSpace/g,kg).replace(/punctSpace/g,$g).replace(/punct/g,qh).getRegex(),Og="^[^_*]*?__[^_*]*?\\*[^_*]*?(?=__)|[^*]+(?=[^*])|(?!\\*)punct(\\*+)(?=[\\s]|$)|notPunctSpace(\\*+)(?!\\*)(?=punctSpace|$)|(?!\\*)[\\s](\\*+)(?=notPunctSpace)|[\\s](\\*+)(?!\\*)(?=punct)|(?!\\*)punct(\\*+)(?!\\*)(?=punct)|(?:(?!\\*)punct|notPunctSpace)(\\*+)(?!\\*)(?=notPunctSpace)",Lg=M(Og,"gu").replace(/notPunctSpace/g,bn).replace(/punctSpace/g,no).replace(/punct/g,Gt).getRegex(),Dg=M("^[^_*]*?\\*\\*[^_*]*?_[^_*]*?(?=\\*\\*)|[^_]+(?=[^_])|(?!_)punct(_+)(?=[\\s]|$)|notPunctSpace(_+)(?!_)(?=punctSpace|$)|(?!_)punctSpace(_+)(?=notPunctSpace)|[\\s](_+)(?!_)(?=punct)|(?!_)punct(_+)(?!_)(?=punct)","gu").replace(/notPunctSpace/g,bn).replace(/punctSpace/g,no).replace(/punct/g,Gt).getRegex(),Ig="^[^_*]*?\\*\\*[^_*]*?_[^_*]*?(?=\\*\\*)|[^_]+(?=[^_])|(?!_)punct(_+)(?=[\\s]|$)|notPunctSpace(_+)(?!_)(?=punctSpace|$)|(?!_)[\\s](_+)(?=notPunctSpace)|[\\s](_+)(?!_)(?=punct)|(?!_)punct(_+)(?!_)(?=punct)|(?:(?!_)punct|notPunctSpace)(_+)(?!_)(?=notPunctSpace)",Hg=M(Ig,"gu").replace(/notPunctSpace/g,bn).replace(/punctSpace/g,no).replace(/punct/g,Gt).getRegex(),zg=M(/^~~?(?:((?!~)punct)|[^\s~])/,"u").replace(/punct/g,Gt).getRegex(),Vg="^[^~]+(?=[^~])|(?!~)punct(~~?)(?=[\\s]|$)|notPunctSpace(~~?)(?!~)(?=punctSpace|$)|(?!~)punctSpace(~~?)(?=notPunctSpace)|[\\s](~~?)(?!~)(?=punct)|(?!~)punct(~~?)(?!~)(?=punct)|notPunctSpace(~~?)(?=notPunctSpace)",Ng=M(Vg,"gu").replace(/notPunctSpace/g,bn).replace(/punctSpace/g,no).replace(/punct/g,Gt).getRegex(),Rg=M(/\\(punct)/,"gu").replace(/punct/g,Gt).getRegex(),Fg=M(/^<(scheme:[^\s\x00-\x1f<>]*|email)>/).replace("scheme",/[a-zA-Z][a-zA-Z0-9+.-]{1,31}/).replace("email",/[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+(@)[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)+(?![-_])/).getRegex(),Bg=M(Dl).replace("(?:-->|$)","-->").getRegex(),Ug=M("^comment|^</[a-zA-Z][\\w:-]*\\s*>|^<[a-zA-Z][\\w-]*(?:attribute)*?\\s*/?>|^<\\?[\\s\\S]*?\\?>|^<![a-zA-Z]+\\s[\\s\\S]*?>|^<!\\[CDATA\\[[\\s\\S]*?\\]\\]>").replace("comment",Bg).replace("attribute",/\s+[a-zA-Z:_][\w.:-]*(?:\s*=\s*"[^"]*"|\s*=\s*'[^']*'|\s*=\s*[^\s"'=<>`]+)?/).getRegex(),us=/(?:\[(?:\\[\s\S]|[^\[\]\\])*\]|\\[\s\S]|`+(?!`)[^`]*?`+(?!`)|``+(?=\])|[^\[\]\\`])*?/,jg=M(/^!?\[(label)\]\(\s*(href)(?:(?:[ \t]+(?:\n[ \t]*)?|\n[ \t]*)(title))?\s*\)/).replace("label",us).replace("href",/<(?:\\.|[^\n<>\\])+>|[^ \t\n\x00-\x1f]+|(?=\))/).replace("title",/"(?:\\"?|[^"\\])*"|'(?:\\'?|[^'\\])*'|\((?:\\\)?|[^)\\])*\)/).getRegex(),Wh=M(/^!?\[(label)\]\[(ref)\]/).replace("label",us).replace("ref",Ll).getRegex(),Yh=M(/^!?\[(ref)\](?:\[\])?/).replace("ref",Ll).getRegex(),qg=M("reflink|nolink(?!\\()","g").replace("reflink",Wh).replace("nolink",Yh).getRegex(),Xc=/[hH][tT][tT][pP][sS]?|[fF][tT][pP]/,Hl={_backpedal:Ii,anyPunctuation:Rg,autolink:Fg,blockSkip:Cg,br:jh,code:_g,del:Ii,delLDelim:Ii,delRDelim:Ii,emStrongLDelim:Sg,emStrongRDelimAst:Tg,emStrongRDelimUnd:Dg,escape:vg,link:jg,nolink:Yh,punctuation:xg,reflink:Wh,reflinkSearch:qg,tag:Ug,text:yg,url:Ii},Gg={...Hl,emStrongLDelim:Ag,emStrongRDelimAst:Lg,emStrongRDelimUnd:Hg,link:M(/^!?\[(label)\]\((.*?)\)/).replace("label",us).getRegex(),reflink:M(/^!?\[(label)\]\s*\[([^\]]*)\]/).replace("label",us).getRegex()},Ua={...Hl,emStrongRDelimAst:Mg,emStrongLDelim:Eg,delLDelim:zg,delRDelim:Ng,url:M(/^((?:protocol):\/\/|www\.)(?:[a-zA-Z0-9\-]+\.?)+[^\s<]*|^email/).replace("protocol",Xc).replace("email",/[A-Za-z0-9._+-]+(@)[a-zA-Z0-9-_]+(?:\.[a-zA-Z0-9-_]*[a-zA-Z0-9])+(?![-_])/).getRegex(),_backpedal:/(?:[^?!.,:;*_'"~()&]+|\([^)]*\)|&(?![a-zA-Z0-9]+;$)|[?!.,:;*_'"~)]+(?!$))+/,del:/^(~~?)(?=[^\s~])((?:\\[\s\S]|[^\\])*?(?:\\[\s\S]|[^\s~\\]))\1(?=[^~]|$)/,text:M(/^(`+|~+|[^`~])(?:(?=[`~])|(?= {2,}\n)|(?=[a-zA-Z0-9.!#$%&'*+\/=?_`{\|}~-]+@)|[\s\S]*?(?:(?=[\\<!\[`*~_]|\b_|protocol:\/\/|www\.|$)|[^ ](?= {2,}\n)|[^a-zA-Z0-9.!#$%&'*+\/=?_`{\|}~-](?=[a-zA-Z0-9.!#$%&'*+\/=?_`{\|}~-]+@)))/).replace("protocol",Xc).getRegex()},Xg={...Ua,br:M(jh).replace("{2,}","*").getRegex(),text:M(Ua.text).replace("\\b_","\\b_| {2,}\\n").replace(/\{2,\}/g,"*").getRegex()},Ln={normal:Il,gfm:mg,pedantic:bg},bo={normal:Hl,gfm:Ua,breaks:Xg,pedantic:Gg},Wg={"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"},Wc=e=>Wg[e];function ct(e,t){if(t){if(le.escapeTest.test(e))return e.replace(le.escapeReplace,Wc)}else if(le.escapeTestNoEncode.test(e))return e.replace(le.escapeReplaceNoEncode,Wc);return e}function Yc(e){try{e=encodeURI(e).replace(le.percentDecode,"%")}catch{return null}return e}function Zc(e,t){var n;let i=e.replace(le.findPipe,(s,a,l)=>{let c=!1,d=a;for(;--d>=0&&l[d]==="\\";)c=!c;return c?"|":" |"}),r=i.split(le.splitPipe),o=0;if(r[0].trim()||r.shift(),r.length>0&&!((n=r.at(-1))!=null&&n.trim())&&r.pop(),t)if(r.length>t)r.splice(t);else for(;r.length<t;)r.push("");for(;o<r.length;o++)r[o]=r[o].trim().replace(le.slashPipe,"|");return r}function Kt(e,t,i){let r=e.length;if(r===0)return"";let o=0;for(;o<r&&e.charAt(r-o-1)===t;)o++;return e.slice(0,r-o)}function Kc(e){let t=e.split(`
`),i=t.length-1;for(;i>=0&&le.blankLine.test(t[i]);)i--;return t.length-i<=2?e:t.slice(0,i+1).join(`
`)}function Yg(e,t){if(e.indexOf(t[1])===-1)return-1;let i=0;for(let r=0;r<e.length;r++)if(e[r]==="\\")r++;else if(e[r]===t[0])i++;else if(e[r]===t[1]&&(i--,i<0))return r;return i>0?-2:-1}function Zg(e,t=0){let i=t,r="";for(let o of e)if(o==="	"){let n=4-i%4;r+=" ".repeat(n),i+=n}else r+=o,i++;return r}function Qc(e,t,i,r,o){let n=t.href,s=t.title||null,a=e[1].replace(o.other.outputLinkReplace,"$1");r.state.inLink=!0;let l={type:e[0].charAt(0)==="!"?"image":"link",raw:i,href:n,title:s,text:a,tokens:r.inlineTokens(a)};return r.state.inLink=!1,l}function Kg(e,t,i){let r=e.match(i.other.indentCodeCompensation);if(r===null)return t;let o=r[1];return t.split(`
`).map(n=>{let s=n.match(i.other.beginningSpace);if(s===null)return n;let[a]=s;return a.length>=o.length?n.slice(o.length):n}).join(`
`)}var fs=class{constructor(e){N(this,"options");N(this,"rules");N(this,"lexer");this.options=e||ur}space(e){let t=this.rules.block.newline.exec(e);if(t&&t[0].length>0)return{type:"space",raw:t[0]}}code(e){let t=this.rules.block.code.exec(e);if(t){let i=this.options.pedantic?t[0]:Kc(t[0]),r=i.replace(this.rules.other.codeRemoveIndent,"");return{type:"code",raw:i,codeBlockStyle:"indented",text:r}}}fences(e){let t=this.rules.block.fences.exec(e);if(t){let i=t[0],r=Kg(i,t[3]||"",this.rules);return{type:"code",raw:i,lang:t[2]?t[2].trim().replace(this.rules.inline.anyPunctuation,"$1"):t[2],text:r}}}heading(e){let t=this.rules.block.heading.exec(e);if(t){let i=t[2].trim();if(this.rules.other.endingHash.test(i)){let r=Kt(i,"#");(this.options.pedantic||!r||this.rules.other.endingSpaceChar.test(r))&&(i=r.trim())}return{type:"heading",raw:Kt(t[0],`
`),depth:t[1].length,text:i,tokens:this.lexer.inline(i)}}}hr(e){let t=this.rules.block.hr.exec(e);if(t)return{type:"hr",raw:Kt(t[0],`
`)}}blockquote(e){let t=this.rules.block.blockquote.exec(e);if(t){let i=Kt(t[0],`
`).split(`
`),r="",o="",n=[];for(;i.length>0;){let s=!1,a=[],l;for(l=0;l<i.length;l++)if(this.rules.other.blockquoteStart.test(i[l]))a.push(i[l]),s=!0;else if(!s)a.push(i[l]);else break;i=i.slice(l);let c=a.join(`
`),d=c.replace(this.rules.other.blockquoteSetextReplace,`
    $1`).replace(this.rules.other.blockquoteSetextReplace2,"");r=r?`${r}
${c}`:c,o=o?`${o}
${d}`:d;let p=this.lexer.state.top;if(this.lexer.state.top=!0,this.lexer.blockTokens(d,n,!0),this.lexer.state.top=p,i.length===0)break;let f=n.at(-1);if((f==null?void 0:f.type)==="code")break;if((f==null?void 0:f.type)==="blockquote"){let b=f,v=i.join(`
`),_=b.raw+`
`+v.replace(this.rules.other.blockquoteSetextReplace2,""),k=this.blockquote(_);n[n.length-1]=k,r=`${r}
${v}`,o=o.substring(0,o.length-b.text.length)+k.text;break}else if((f==null?void 0:f.type)==="list"){let b=f,v=b.raw+`
`+i.join(`
`),_=this.list(v);n[n.length-1]=_,r=r.substring(0,r.length-f.raw.length)+_.raw,o=o.substring(0,o.length-b.raw.length)+_.raw,i=v.substring(n.at(-1).raw.length).split(`
`);continue}}return{type:"blockquote",raw:r,tokens:n,text:o}}}list(e){let t=this.rules.block.list.exec(e);if(t){let i=t[1].trim(),r=i.length>1,o={type:"list",raw:"",ordered:r,start:r?+i.slice(0,-1):"",loose:!1,items:[]};i=r?`\\d{1,9}\\${i.slice(-1)}`:`\\${i}`,this.options.pedantic&&(i=r?i:"[*+-]");let n=this.rules.other.listItemRegex(i),s=!1;for(;e;){let l=!1,c="",d="";if(!(t=n.exec(e))||this.rules.block.hr.test(e))break;c=t[0],e=e.substring(c.length);let p=Zg(t[2].split(`
`,1)[0],t[1].length),f=e.split(`
`,1)[0],b=!p.trim(),v=0;if(this.options.pedantic?(v=2,d=p.trimStart()):b?v=t[1].length+1:(v=p.search(this.rules.other.nonSpaceChar),v=v>4?1:v,d=p.slice(v),v+=t[1].length),b&&this.rules.other.blankLine.test(f)&&(c+=f+`
`,e=e.substring(f.length+1),l=!0),!l){let _=this.rules.other.nextBulletRegex(v),k=this.rules.other.hrRegex(v),C=this.rules.other.fencesBeginRegex(v),O=this.rules.other.headingBeginRegex(v),I=this.rules.other.htmlBeginRegex(v),V=this.rules.other.blockquoteBeginRegex(v);for(;e;){let K=e.split(`
`,1)[0],R;if(f=K,this.options.pedantic?(f=f.replace(this.rules.other.listReplaceNesting,"  "),R=f):R=f.replace(this.rules.other.tabCharGlobal,"    "),C.test(f)||O.test(f)||I.test(f)||V.test(f)||_.test(f)||k.test(f))break;if(R.search(this.rules.other.nonSpaceChar)>=v||!f.trim())d+=`
`+R.slice(v);else{if(b||p.replace(this.rules.other.tabCharGlobal,"    ").search(this.rules.other.nonSpaceChar)>=4||C.test(p)||O.test(p)||k.test(p))break;d+=`
`+f}b=!f.trim(),c+=K+`
`,e=e.substring(K.length+1),p=R.slice(v)}}o.loose||(s?o.loose=!0:this.rules.other.doubleBlankLine.test(c)&&(s=!0)),o.items.push({type:"list_item",raw:c,task:!!this.options.gfm&&this.rules.other.listIsTask.test(d),loose:!1,text:d,tokens:[]}),o.raw+=c}let a=o.items.at(-1);if(a)a.raw=a.raw.trimEnd(),a.text=a.text.trimEnd();else return;o.raw=o.raw.trimEnd();for(let l of o.items)if(this.lexer.state.top=!1,l.tokens=this.lexer.blockTokens(l.text,[]),!o.loose){let c=l.tokens.filter(p=>p.type==="space"),d=c.length>0&&c.some(p=>this.rules.other.anyLine.test(p.raw));o.loose=d}for(let l of o.items){let c=l.tokens[0];if(l.task&&((c==null?void 0:c.type)==="text"||(c==null?void 0:c.type)==="paragraph")){l.text=l.text.replace(this.rules.other.listReplaceTask,""),c.raw=c.raw.replace(this.rules.other.listReplaceTask,""),c.text=c.text.replace(this.rules.other.listReplaceTask,"");for(let p=this.lexer.inlineQueue.length-1;p>=0;p--)if(this.rules.other.listIsTask.test(this.lexer.inlineQueue[p].src)){this.lexer.inlineQueue[p].src=this.lexer.inlineQueue[p].src.replace(this.rules.other.listReplaceTask,"");break}let d=this.rules.other.listTaskCheckbox.exec(l.raw);if(d){let p={type:"checkbox",raw:d[0]+" ",checked:d[0]!=="[ ]"};l.checked=p.checked,o.loose?l.tokens[0]&&["paragraph","text"].includes(l.tokens[0].type)&&"tokens"in l.tokens[0]&&l.tokens[0].tokens?(l.tokens[0].raw=p.raw+l.tokens[0].raw,l.tokens[0].text=p.raw+l.tokens[0].text,l.tokens[0].tokens.unshift(p)):l.tokens.unshift({type:"paragraph",raw:p.raw,text:p.raw,tokens:[p]}):l.tokens.unshift(p)}}else l.task&&(l.task=!1)}if(o.loose)for(let l of o.items){l.loose=!0;for(let c of l.tokens)c.type==="text"&&(c.type="paragraph")}return o}}html(e){let t=this.rules.block.html.exec(e);if(t){let i=Kc(t[0]);return{type:"html",block:!0,raw:i,pre:t[1]==="pre"||t[1]==="script"||t[1]==="style",text:i}}}def(e){let t=this.rules.block.def.exec(e);if(t){let i=t[1].toLowerCase().replace(this.rules.other.multipleSpaceGlobal," "),r=t[2]?t[2].replace(this.rules.other.hrefBrackets,"$1").replace(this.rules.inline.anyPunctuation,"$1"):"",o=t[3]?t[3].substring(1,t[3].length-1).replace(this.rules.inline.anyPunctuation,"$1"):t[3];return{type:"def",tag:i,raw:Kt(t[0],`
`),href:r,title:o}}}table(e){var s;let t=this.rules.block.table.exec(e);if(!t||!this.rules.other.tableDelimiter.test(t[2]))return;let i=Zc(t[1]),r=t[2].replace(this.rules.other.tableAlignChars,"").split("|"),o=(s=t[3])!=null&&s.trim()?t[3].replace(this.rules.other.tableRowBlankLine,"").split(`
`):[],n={type:"table",raw:Kt(t[0],`
`),header:[],align:[],rows:[]};if(i.length===r.length){for(let a of r)this.rules.other.tableAlignRight.test(a)?n.align.push("right"):this.rules.other.tableAlignCenter.test(a)?n.align.push("center"):this.rules.other.tableAlignLeft.test(a)?n.align.push("left"):n.align.push(null);for(let a=0;a<i.length;a++)n.header.push({text:i[a],tokens:this.lexer.inline(i[a]),header:!0,align:n.align[a]});for(let a of o)n.rows.push(Zc(a,n.header.length).map((l,c)=>({text:l,tokens:this.lexer.inline(l),header:!1,align:n.align[c]})));return n}}lheading(e){let t=this.rules.block.lheading.exec(e);if(t){let i=t[1].trim();return{type:"heading",raw:Kt(t[0],`
`),depth:t[2].charAt(0)==="="?1:2,text:i,tokens:this.lexer.inline(i)}}}paragraph(e){let t=this.rules.block.paragraph.exec(e);if(t){let i=t[1].charAt(t[1].length-1)===`
`?t[1].slice(0,-1):t[1];return{type:"paragraph",raw:t[0],text:i,tokens:this.lexer.inline(i)}}}text(e){let t=this.rules.block.text.exec(e);if(t)return{type:"text",raw:t[0],text:t[0],tokens:this.lexer.inline(t[0])}}escape(e){let t=this.rules.inline.escape.exec(e);if(t)return{type:"escape",raw:t[0],text:t[1]}}tag(e){let t=this.rules.inline.tag.exec(e);if(t)return!this.lexer.state.inLink&&this.rules.other.startATag.test(t[0])?this.lexer.state.inLink=!0:this.lexer.state.inLink&&this.rules.other.endATag.test(t[0])&&(this.lexer.state.inLink=!1),!this.lexer.state.inRawBlock&&this.rules.other.startPreScriptTag.test(t[0])?this.lexer.state.inRawBlock=!0:this.lexer.state.inRawBlock&&this.rules.other.endPreScriptTag.test(t[0])&&(this.lexer.state.inRawBlock=!1),{type:"html",raw:t[0],inLink:this.lexer.state.inLink,inRawBlock:this.lexer.state.inRawBlock,block:!1,text:t[0]}}link(e){let t=this.rules.inline.link.exec(e);if(t){let i=t[2].trim();if(!this.options.pedantic&&this.rules.other.startAngleBracket.test(i)){if(!this.rules.other.endAngleBracket.test(i))return;let n=Kt(i.slice(0,-1),"\\");if((i.length-n.length)%2===0)return}else{let n=Yg(t[2],"()");if(n===-2)return;if(n>-1){let s=(t[0].indexOf("!")===0?5:4)+t[1].length+n;t[2]=t[2].substring(0,n),t[0]=t[0].substring(0,s).trim(),t[3]=""}}let r=t[2],o="";if(this.options.pedantic){let n=this.rules.other.pedanticHrefTitle.exec(r);n&&(r=n[1],o=n[3])}else o=t[3]?t[3].slice(1,-1):"";return r=r.trim(),this.rules.other.startAngleBracket.test(r)&&(this.options.pedantic&&!this.rules.other.endAngleBracket.test(i)?r=r.slice(1):r=r.slice(1,-1)),Qc(t,{href:r&&r.replace(this.rules.inline.anyPunctuation,"$1"),title:o&&o.replace(this.rules.inline.anyPunctuation,"$1")},t[0],this.lexer,this.rules)}}reflink(e,t){let i;if((i=this.rules.inline.reflink.exec(e))||(i=this.rules.inline.nolink.exec(e))){let r=(i[2]||i[1]).replace(this.rules.other.multipleSpaceGlobal," "),o=t[r.toLowerCase()];if(!o){let n=i[0].charAt(0);return{type:"text",raw:n,text:n}}return Qc(i,o,i[0],this.lexer,this.rules)}}emStrong(e,t,i=""){let r=this.rules.inline.emStrongLDelim.exec(e);if(!(!r||!r[1]&&!r[2]&&!r[3]&&!r[4]||r[4]&&i.match(this.rules.other.unicodeAlphaNumeric))&&(!(r[1]||r[3])||!i||this.rules.inline.punctuation.exec(i))){let o=[...r[0]].length-1,n,s,a=o,l=0,c=r[0][0],d=i===c,p=c==="*"?this.rules.inline.emStrongRDelimAst:this.rules.inline.emStrongRDelimUnd;for(p.lastIndex=0,t=t.slice(-1*e.length+o);(r=p.exec(t))!==null;){if(n=r[1]||r[2]||r[3]||r[4]||r[5]||r[6],!n)continue;if(s=[...n].length,r[3]||r[4]){a+=s;continue}else if(r[5]||r[6]){if(o%3&&!((o+s)%3)){l+=s;continue}if(d)break}if(a-=s,a>0)continue;s=Math.min(s,s+a+l);let f=[...r[0]][0].length,b=e.slice(0,o+r.index+f+s);if(Math.min(o,s)%2){let _=b.slice(1,-1);return{type:"em",raw:b,text:_,tokens:this.lexer.inlineTokens(_)}}let v=b.slice(2,-2);return{type:"strong",raw:b,text:v,tokens:this.lexer.inlineTokens(v)}}}}codespan(e){let t=this.rules.inline.code.exec(e);if(t){let i=t[2].replace(this.rules.other.newLineCharGlobal," "),r=this.rules.other.nonSpaceChar.test(i),o=this.rules.other.startingSpaceChar.test(i)&&this.rules.other.endingSpaceChar.test(i);return r&&o&&(i=i.substring(1,i.length-1)),{type:"codespan",raw:t[0],text:i}}}br(e){let t=this.rules.inline.br.exec(e);if(t)return{type:"br",raw:t[0]}}del(e,t,i=""){let r=this.rules.inline.delLDelim.exec(e);if(r&&(!r[1]||!i||this.rules.inline.punctuation.exec(i))){let o=[...r[0]].length-1,n,s,a=o,l=this.rules.inline.delRDelim;for(l.lastIndex=0,t=t.slice(-1*e.length+o);(r=l.exec(t))!==null;){if(n=r[1]||r[2]||r[3]||r[4]||r[5]||r[6],!n||(s=[...n].length,s!==o))continue;if(r[3]||r[4]){a+=s;continue}if(a-=s,a>0)continue;s=Math.min(s,s+a);let c=[...r[0]][0].length,d=e.slice(0,o+r.index+c+s),p=d.slice(o,-o);return{type:"del",raw:d,text:p,tokens:this.lexer.inlineTokens(p)}}}}autolink(e){let t=this.rules.inline.autolink.exec(e);if(t){let i,r;return t[2]==="@"?(i=t[1],r="mailto:"+i):(i=t[1],r=i),{type:"link",raw:t[0],text:i,href:r,tokens:[{type:"text",raw:i,text:i}]}}}url(e){var i;let t;if(t=this.rules.inline.url.exec(e)){let r,o;if(t[2]==="@")r=t[0],o="mailto:"+r;else{let n;do n=t[0],t[0]=((i=this.rules.inline._backpedal.exec(t[0]))==null?void 0:i[0])??"";while(n!==t[0]);r=t[0],t[1]==="www."?o="http://"+t[0]:o=t[0]}return{type:"link",raw:t[0],text:r,href:o,tokens:[{type:"text",raw:r,text:r}]}}}inlineText(e){let t=this.rules.inline.text.exec(e);if(t){let i=this.lexer.state.inRawBlock;return{type:"text",raw:t[0],text:t[0],escaped:i}}}},Je=class ja{constructor(t){N(this,"tokens");N(this,"options");N(this,"state");N(this,"inlineQueue");N(this,"tokenizer");this.tokens=[],this.tokens.links=Object.create(null),this.options=t||ur,this.options.tokenizer=this.options.tokenizer||new fs,this.tokenizer=this.options.tokenizer,this.tokenizer.options=this.options,this.tokenizer.lexer=this,this.inlineQueue=[],this.state={inLink:!1,inRawBlock:!1,top:!0};let i={other:le,block:Ln.normal,inline:bo.normal};this.options.pedantic?(i.block=Ln.pedantic,i.inline=bo.pedantic):this.options.gfm&&(i.block=Ln.gfm,this.options.breaks?i.inline=bo.breaks:i.inline=bo.gfm),this.tokenizer.rules=i}static get rules(){return{block:Ln,inline:bo}}static lex(t,i){return new ja(i).lex(t)}static lexInline(t,i){return new ja(i).inlineTokens(t)}lex(t){t=t.replace(le.carriageReturn,`
`),this.blockTokens(t,this.tokens);for(let i=0;i<this.inlineQueue.length;i++){let r=this.inlineQueue[i];this.inlineTokens(r.src,r.tokens)}return this.inlineQueue=[],this.tokens}blockTokens(t,i=[],r=!1){var n,s,a;this.tokenizer.lexer=this,this.options.pedantic&&(t=t.replace(le.tabCharGlobal,"    ").replace(le.spaceLine,""));let o=1/0;for(;t;){if(t.length<o)o=t.length;else{this.infiniteLoopError(t.charCodeAt(0));break}let l;if((s=(n=this.options.extensions)==null?void 0:n.block)!=null&&s.some(d=>(l=d.call({lexer:this},t,i))?(t=t.substring(l.raw.length),i.push(l),!0):!1))continue;if(l=this.tokenizer.space(t)){t=t.substring(l.raw.length);let d=i.at(-1);l.raw.length===1&&d!==void 0?d.raw+=`
`:i.push(l);continue}if(l=this.tokenizer.code(t)){t=t.substring(l.raw.length);let d=i.at(-1);(d==null?void 0:d.type)==="paragraph"||(d==null?void 0:d.type)==="text"?(d.raw+=(d.raw.endsWith(`
`)?"":`
`)+l.raw,d.text+=`
`+l.text,this.inlineQueue.at(-1).src=d.text):i.push(l);continue}if(l=this.tokenizer.fences(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.heading(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.hr(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.blockquote(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.list(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.html(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.def(t)){t=t.substring(l.raw.length);let d=i.at(-1);(d==null?void 0:d.type)==="paragraph"||(d==null?void 0:d.type)==="text"?(d.raw+=(d.raw.endsWith(`
`)?"":`
`)+l.raw,d.text+=`
`+l.raw,this.inlineQueue.at(-1).src=d.text):this.tokens.links[l.tag]||(this.tokens.links[l.tag]={href:l.href,title:l.title},i.push(l));continue}if(l=this.tokenizer.table(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.lheading(t)){t=t.substring(l.raw.length),i.push(l);continue}let c=t;if((a=this.options.extensions)!=null&&a.startBlock){let d=1/0,p=t.slice(1),f;this.options.extensions.startBlock.forEach(b=>{f=b.call({lexer:this},p),typeof f=="number"&&f>=0&&(d=Math.min(d,f))}),d<1/0&&d>=0&&(c=t.substring(0,d+1))}if(this.state.top&&(l=this.tokenizer.paragraph(c))){let d=i.at(-1);r&&(d==null?void 0:d.type)==="paragraph"?(d.raw+=(d.raw.endsWith(`
`)?"":`
`)+l.raw,d.text+=`
`+l.text,this.inlineQueue.pop(),this.inlineQueue.at(-1).src=d.text):i.push(l),r=c.length!==t.length,t=t.substring(l.raw.length);continue}if(l=this.tokenizer.text(t)){t=t.substring(l.raw.length);let d=i.at(-1);(d==null?void 0:d.type)==="text"?(d.raw+=(d.raw.endsWith(`
`)?"":`
`)+l.raw,d.text+=`
`+l.text,this.inlineQueue.pop(),this.inlineQueue.at(-1).src=d.text):i.push(l);continue}if(t){this.infiniteLoopError(t.charCodeAt(0));break}}return this.state.top=!0,i}inline(t,i=[]){return this.inlineQueue.push({src:t,tokens:i}),i}inlineTokens(t,i=[]){var a,l,c,d,p;this.tokenizer.lexer=this;let r=t;if(this.tokens.links){let f=Object.keys(this.tokens.links);f.length>0&&(r=r.replace(this.tokenizer.rules.inline.reflinkSearch,b=>f.includes(b.slice(b.lastIndexOf("[")+1,-1))?"["+"a".repeat(b.length-2)+"]":b))}r=r.replace(this.tokenizer.rules.inline.anyPunctuation,f=>"+".repeat(f.length)),r=r.replace(this.tokenizer.rules.inline.blockSkip,(f,b,v)=>{let _=v?v.length:0;return f.slice(0,_)+"["+"a".repeat(f.length-_-2)+"]"}),r=((l=(a=this.options.hooks)==null?void 0:a.emStrongMask)==null?void 0:l.call({lexer:this},r))??r;let o=!1,n="",s=1/0;for(;t;){if(t.length<s)s=t.length;else{this.infiniteLoopError(t.charCodeAt(0));break}o||(n=""),o=!1;let f;if((d=(c=this.options.extensions)==null?void 0:c.inline)!=null&&d.some(v=>(f=v.call({lexer:this},t,i))?(t=t.substring(f.raw.length),i.push(f),!0):!1))continue;if(f=this.tokenizer.escape(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.tag(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.link(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.reflink(t,this.tokens.links)){t=t.substring(f.raw.length);let v=i.at(-1);f.type==="text"&&(v==null?void 0:v.type)==="text"?(v.raw+=f.raw,v.text+=f.text):i.push(f);continue}if(f=this.tokenizer.emStrong(t,r,n)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.codespan(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.br(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.del(t,r,n)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.autolink(t)){t=t.substring(f.raw.length),i.push(f);continue}if(!this.state.inLink&&(f=this.tokenizer.url(t))){t=t.substring(f.raw.length),i.push(f);continue}let b=t;if((p=this.options.extensions)!=null&&p.startInline){let v=1/0,_=t.slice(1),k;this.options.extensions.startInline.forEach(C=>{k=C.call({lexer:this},_),typeof k=="number"&&k>=0&&(v=Math.min(v,k))}),v<1/0&&v>=0&&(b=t.substring(0,v+1))}if(f=this.tokenizer.inlineText(b)){t=t.substring(f.raw.length),f.raw.slice(-1)!=="_"&&(n=f.raw.slice(-1)),o=!0;let v=i.at(-1);(v==null?void 0:v.type)==="text"?(v.raw+=f.raw,v.text+=f.text):i.push(f);continue}if(t){this.infiniteLoopError(t.charCodeAt(0));break}}return i}infiniteLoopError(t){let i="Infinite loop on byte: "+t;if(this.options.silent)console.error(i);else throw new Error(i)}},gs=class{constructor(e){N(this,"options");N(this,"parser");this.options=e||ur}space(e){return""}code({text:e,lang:t,escaped:i}){var n;let r=(n=(t||"").match(le.notSpaceStart))==null?void 0:n[0],o=e.replace(le.endingNewline,"")+`
`;return r?'<pre><code class="language-'+ct(r)+'">'+(i?o:ct(o,!0))+`</code></pre>
`:"<pre><code>"+(i?o:ct(o,!0))+`</code></pre>
`}blockquote({tokens:e}){return`<blockquote>
${this.parser.parse(e)}</blockquote>
`}html({text:e}){return e}def(e){return""}heading({tokens:e,depth:t}){return`<h${t}>${this.parser.parseInline(e)}</h${t}>
`}hr(e){return`<hr>
`}list(e){let t=e.ordered,i=e.start,r="";for(let s=0;s<e.items.length;s++){let a=e.items[s];r+=this.listitem(a)}let o=t?"ol":"ul",n=t&&i!==1?' start="'+i+'"':"";return"<"+o+n+`>
`+r+"</"+o+`>
`}listitem(e){return`<li>${this.parser.parse(e.tokens)}</li>
`}checkbox({checked:e}){return"<input "+(e?'checked="" ':"")+'disabled="" type="checkbox"> '}paragraph({tokens:e}){return`<p>${this.parser.parseInline(e)}</p>
`}table(e){let t="",i="";for(let o=0;o<e.header.length;o++)i+=this.tablecell(e.header[o]);t+=this.tablerow({text:i});let r="";for(let o=0;o<e.rows.length;o++){let n=e.rows[o];i="";for(let s=0;s<n.length;s++)i+=this.tablecell(n[s]);r+=this.tablerow({text:i})}return r&&(r=`<tbody>${r}</tbody>`),`<table>
<thead>
`+t+`</thead>
`+r+`</table>
`}tablerow({text:e}){return`<tr>
${e}</tr>
`}tablecell(e){let t=this.parser.parseInline(e.tokens),i=e.header?"th":"td";return(e.align?`<${i} align="${e.align}">`:`<${i}>`)+t+`</${i}>
`}strong({tokens:e}){return`<strong>${this.parser.parseInline(e)}</strong>`}em({tokens:e}){return`<em>${this.parser.parseInline(e)}</em>`}codespan({text:e}){return`<code>${ct(e,!0)}</code>`}br(e){return"<br>"}del({tokens:e}){return`<del>${this.parser.parseInline(e)}</del>`}link({href:e,title:t,tokens:i}){let r=this.parser.parseInline(i),o=Yc(e);if(o===null)return r;e=o;let n='<a href="'+e+'"';return t&&(n+=' title="'+ct(t)+'"'),n+=">"+r+"</a>",n}image({href:e,title:t,text:i,tokens:r}){r&&(i=this.parser.parseInline(r,this.parser.textRenderer));let o=Yc(e);if(o===null)return ct(i);e=o;let n=`<img src="${e}" alt="${ct(i)}"`;return t&&(n+=` title="${ct(t)}"`),n+=">",n}text(e){return"tokens"in e&&e.tokens?this.parser.parseInline(e.tokens):"escaped"in e&&e.escaped?e.text:ct(e.text)}},zl=class{strong({text:e}){return e}em({text:e}){return e}codespan({text:e}){return e}del({text:e}){return e}html({text:e}){return e}text({text:e}){return e}link({text:e}){return""+e}image({text:e}){return""+e}br(){return""}checkbox({raw:e}){return e}},et=class qa{constructor(t){N(this,"options");N(this,"renderer");N(this,"textRenderer");this.options=t||ur,this.options.renderer=this.options.renderer||new gs,this.renderer=this.options.renderer,this.renderer.options=this.options,this.renderer.parser=this,this.textRenderer=new zl}static parse(t,i){return new qa(i).parse(t)}static parseInline(t,i){return new qa(i).parseInline(t)}parse(t){var r,o;this.renderer.parser=this;let i="";for(let n=0;n<t.length;n++){let s=t[n];if((o=(r=this.options.extensions)==null?void 0:r.renderers)!=null&&o[s.type]){let l=s,c=this.options.extensions.renderers[l.type].call({parser:this},l);if(c!==!1||!["space","hr","heading","code","table","blockquote","list","checkbox","html","def","paragraph","text"].includes(l.type)){i+=c||"";continue}}let a=s;switch(a.type){case"space":{i+=this.renderer.space(a);break}case"hr":{i+=this.renderer.hr(a);break}case"heading":{i+=this.renderer.heading(a);break}case"code":{i+=this.renderer.code(a);break}case"table":{i+=this.renderer.table(a);break}case"blockquote":{i+=this.renderer.blockquote(a);break}case"list":{i+=this.renderer.list(a);break}case"checkbox":{i+=this.renderer.checkbox(a);break}case"html":{i+=this.renderer.html(a);break}case"def":{i+=this.renderer.def(a);break}case"paragraph":{i+=this.renderer.paragraph(a);break}case"text":{i+=this.renderer.text(a);break}default:{let l='Token with "'+a.type+'" type was not found.';if(this.options.silent)return console.error(l),"";throw new Error(l)}}}return i}parseInline(t,i=this.renderer){var o,n;this.renderer.parser=this;let r="";for(let s=0;s<t.length;s++){let a=t[s];if((n=(o=this.options.extensions)==null?void 0:o.renderers)!=null&&n[a.type]){let c=this.options.extensions.renderers[a.type].call({parser:this},a);if(c!==!1||!["escape","html","link","image","checkbox","strong","em","codespan","br","del","text"].includes(a.type)){r+=c||"";continue}}let l=a;switch(l.type){case"escape":{r+=i.text(l);break}case"html":{r+=i.html(l);break}case"link":{r+=i.link(l);break}case"image":{r+=i.image(l);break}case"checkbox":{r+=i.checkbox(l);break}case"strong":{r+=i.strong(l);break}case"em":{r+=i.em(l);break}case"codespan":{r+=i.codespan(l);break}case"br":{r+=i.br(l);break}case"del":{r+=i.del(l);break}case"text":{r+=i.text(l);break}default:{let c='Token with "'+l.type+'" type was not found.';if(this.options.silent)return console.error(c),"";throw new Error(c)}}}return r}},Wn,$o=(Wn=class{constructor(e){N(this,"options");N(this,"block");this.options=e||ur}preprocess(e){return e}postprocess(e){return e}processAllTokens(e){return e}emStrongMask(e){return e}provideLexer(e=this.block){return e?Je.lex:Je.lexInline}provideParser(e=this.block){return e?et.parse:et.parseInline}},N(Wn,"passThroughHooks",new Set(["preprocess","postprocess","processAllTokens","emStrongMask"])),N(Wn,"passThroughHooksRespectAsync",new Set(["preprocess","postprocess","processAllTokens"])),Wn),Qg=class{constructor(...e){N(this,"defaults",Tl());N(this,"options",this.setOptions);N(this,"parse",this.parseMarkdown(!0));N(this,"parseInline",this.parseMarkdown(!1));N(this,"Parser",et);N(this,"Renderer",gs);N(this,"TextRenderer",zl);N(this,"Lexer",Je);N(this,"Tokenizer",fs);N(this,"Hooks",$o);this.use(...e)}walkTokens(e,t){var r,o;let i=[];for(let n of e)switch(i=i.concat(t.call(this,n)),n.type){case"table":{let s=n;for(let a of s.header)i=i.concat(this.walkTokens(a.tokens,t));for(let a of s.rows)for(let l of a)i=i.concat(this.walkTokens(l.tokens,t));break}case"list":{let s=n;i=i.concat(this.walkTokens(s.items,t));break}default:{let s=n;(o=(r=this.defaults.extensions)==null?void 0:r.childTokens)!=null&&o[s.type]?this.defaults.extensions.childTokens[s.type].forEach(a=>{let l=s[a].flat(1/0);i=i.concat(this.walkTokens(l,t))}):s.tokens&&(i=i.concat(this.walkTokens(s.tokens,t)))}}return i}use(...e){let t=this.defaults.extensions||{renderers:{},childTokens:{}};return e.forEach(i=>{let r={...i};if(r.async=this.defaults.async||r.async||!1,i.extensions&&(i.extensions.forEach(o=>{if(!o.name)throw new Error("extension name required");if("renderer"in o){let n=t.renderers[o.name];n?t.renderers[o.name]=function(...s){let a=o.renderer.apply(this,s);return a===!1&&(a=n.apply(this,s)),a}:t.renderers[o.name]=o.renderer}if("tokenizer"in o){if(!o.level||o.level!=="block"&&o.level!=="inline")throw new Error("extension level must be 'block' or 'inline'");let n=t[o.level];n?n.unshift(o.tokenizer):t[o.level]=[o.tokenizer],o.start&&(o.level==="block"?t.startBlock?t.startBlock.push(o.start):t.startBlock=[o.start]:o.level==="inline"&&(t.startInline?t.startInline.push(o.start):t.startInline=[o.start]))}"childTokens"in o&&o.childTokens&&(t.childTokens[o.name]=o.childTokens)}),r.extensions=t),i.renderer){let o=this.defaults.renderer||new gs(this.defaults);for(let n in i.renderer){if(!(n in o))throw new Error(`renderer '${n}' does not exist`);if(["options","parser"].includes(n))continue;let s=n,a=i.renderer[s],l=o[s];o[s]=(...c)=>{let d=a.apply(o,c);return d===!1&&(d=l.apply(o,c)),d||""}}r.renderer=o}if(i.tokenizer){let o=this.defaults.tokenizer||new fs(this.defaults);for(let n in i.tokenizer){if(!(n in o))throw new Error(`tokenizer '${n}' does not exist`);if(["options","rules","lexer"].includes(n))continue;let s=n,a=i.tokenizer[s],l=o[s];o[s]=(...c)=>{let d=a.apply(o,c);return d===!1&&(d=l.apply(o,c)),d}}r.tokenizer=o}if(i.hooks){let o=this.defaults.hooks||new $o;for(let n in i.hooks){if(!(n in o))throw new Error(`hook '${n}' does not exist`);if(["options","block"].includes(n))continue;let s=n,a=i.hooks[s],l=o[s];$o.passThroughHooks.has(n)?o[s]=c=>{if(this.defaults.async&&$o.passThroughHooksRespectAsync.has(n))return(async()=>{let p=await a.call(o,c);return l.call(o,p)})();let d=a.call(o,c);return l.call(o,d)}:o[s]=(...c)=>{if(this.defaults.async)return(async()=>{let p=await a.apply(o,c);return p===!1&&(p=await l.apply(o,c)),p})();let d=a.apply(o,c);return d===!1&&(d=l.apply(o,c)),d}}r.hooks=o}if(i.walkTokens){let o=this.defaults.walkTokens,n=i.walkTokens;r.walkTokens=function(s){let a=[];return a.push(n.call(this,s)),o&&(a=a.concat(o.call(this,s))),a}}this.defaults={...this.defaults,...r}}),this}setOptions(e){return this.defaults={...this.defaults,...e},this}lexer(e,t){return Je.lex(e,t??this.defaults)}parser(e,t){return et.parse(e,t??this.defaults)}parseMarkdown(e){return(t,i)=>{let r={...i},o={...this.defaults,...r},n=this.onError(!!o.silent,!!o.async);if(this.defaults.async===!0&&r.async===!1)return n(new Error("marked(): The async option was set to true by an extension. Remove async: false from the parse options object to return a Promise."));if(typeof t>"u"||t===null)return n(new Error("marked(): input parameter is undefined or null"));if(typeof t!="string")return n(new Error("marked(): input parameter is of type "+Object.prototype.toString.call(t)+", string expected"));if(o.hooks&&(o.hooks.options=o,o.hooks.block=e),o.async)return(async()=>{let s=o.hooks?await o.hooks.preprocess(t):t,a=await(o.hooks?await o.hooks.provideLexer(e):e?Je.lex:Je.lexInline)(s,o),l=o.hooks?await o.hooks.processAllTokens(a):a;o.walkTokens&&await Promise.all(this.walkTokens(l,o.walkTokens));let c=await(o.hooks?await o.hooks.provideParser(e):e?et.parse:et.parseInline)(l,o);return o.hooks?await o.hooks.postprocess(c):c})().catch(n);try{o.hooks&&(t=o.hooks.preprocess(t));let s=(o.hooks?o.hooks.provideLexer(e):e?Je.lex:Je.lexInline)(t,o);o.hooks&&(s=o.hooks.processAllTokens(s)),o.walkTokens&&this.walkTokens(s,o.walkTokens);let a=(o.hooks?o.hooks.provideParser(e):e?et.parse:et.parseInline)(s,o);return o.hooks&&(a=o.hooks.postprocess(a)),a}catch(s){return n(s)}}}onError(e,t){return i=>{if(i.message+=`
Please report this to https://github.com/markedjs/marked.`,e){let r="<p>An error occurred:</p><pre>"+ct(i.message+"",!0)+"</pre>";return t?Promise.resolve(r):r}if(t)return Promise.reject(i);throw i}}},Xi=new Qg;function H(e,t){return Xi.parse(e,t)}H.options=H.setOptions=function(e){return Xi.setOptions(e),H.defaults=Xi.defaults,Rh(H.defaults),H};H.getDefaults=Tl;H.defaults=ur;function Jg(...e){return Xi.use(...e),H.defaults=Xi.defaults,Rh(H.defaults),H}H.use=Jg;H.walkTokens=function(e,t){return Xi.walkTokens(e,t)};H.parseInline=Xi.parseInline;H.Parser=et;H.parser=et.parse;H.Renderer=gs;H.TextRenderer=zl;H.Lexer=Je;H.lexer=Je.lex;H.Tokenizer=fs;H.Hooks=$o;H.parse=H;H.options;H.setOptions;H.walkTokens;H.parseInline;et.parse;Je.lex;var Ga={exports:{}},q={},Xa={exports:{}},fr={};function Zh(){var e={};return e["align-content"]=!1,e["align-items"]=!1,e["align-self"]=!1,e["alignment-adjust"]=!1,e["alignment-baseline"]=!1,e.all=!1,e["anchor-point"]=!1,e.animation=!1,e["animation-delay"]=!1,e["animation-direction"]=!1,e["animation-duration"]=!1,e["animation-fill-mode"]=!1,e["animation-iteration-count"]=!1,e["animation-name"]=!1,e["animation-play-state"]=!1,e["animation-timing-function"]=!1,e.azimuth=!1,e["backface-visibility"]=!1,e.background=!0,e["background-attachment"]=!0,e["background-clip"]=!0,e["background-color"]=!0,e["background-image"]=!0,e["background-origin"]=!0,e["background-position"]=!0,e["background-repeat"]=!0,e["background-size"]=!0,e["baseline-shift"]=!1,e.binding=!1,e.bleed=!1,e["bookmark-label"]=!1,e["bookmark-level"]=!1,e["bookmark-state"]=!1,e.border=!0,e["border-bottom"]=!0,e["border-bottom-color"]=!0,e["border-bottom-left-radius"]=!0,e["border-bottom-right-radius"]=!0,e["border-bottom-style"]=!0,e["border-bottom-width"]=!0,e["border-collapse"]=!0,e["border-color"]=!0,e["border-image"]=!0,e["border-image-outset"]=!0,e["border-image-repeat"]=!0,e["border-image-slice"]=!0,e["border-image-source"]=!0,e["border-image-width"]=!0,e["border-left"]=!0,e["border-left-color"]=!0,e["border-left-style"]=!0,e["border-left-width"]=!0,e["border-radius"]=!0,e["border-right"]=!0,e["border-right-color"]=!0,e["border-right-style"]=!0,e["border-right-width"]=!0,e["border-spacing"]=!0,e["border-style"]=!0,e["border-top"]=!0,e["border-top-color"]=!0,e["border-top-left-radius"]=!0,e["border-top-right-radius"]=!0,e["border-top-style"]=!0,e["border-top-width"]=!0,e["border-width"]=!0,e.bottom=!1,e["box-decoration-break"]=!0,e["box-shadow"]=!0,e["box-sizing"]=!0,e["box-snap"]=!0,e["box-suppress"]=!0,e["break-after"]=!0,e["break-before"]=!0,e["break-inside"]=!0,e["caption-side"]=!1,e.chains=!1,e.clear=!0,e.clip=!1,e["clip-path"]=!1,e["clip-rule"]=!1,e.color=!0,e["color-interpolation-filters"]=!0,e["column-count"]=!1,e["column-fill"]=!1,e["column-gap"]=!1,e["column-rule"]=!1,e["column-rule-color"]=!1,e["column-rule-style"]=!1,e["column-rule-width"]=!1,e["column-span"]=!1,e["column-width"]=!1,e.columns=!1,e.contain=!1,e.content=!1,e["counter-increment"]=!1,e["counter-reset"]=!1,e["counter-set"]=!1,e.crop=!1,e.cue=!1,e["cue-after"]=!1,e["cue-before"]=!1,e.cursor=!1,e.direction=!1,e.display=!0,e["display-inside"]=!0,e["display-list"]=!0,e["display-outside"]=!0,e["dominant-baseline"]=!1,e.elevation=!1,e["empty-cells"]=!1,e.filter=!1,e.flex=!1,e["flex-basis"]=!1,e["flex-direction"]=!1,e["flex-flow"]=!1,e["flex-grow"]=!1,e["flex-shrink"]=!1,e["flex-wrap"]=!1,e.float=!1,e["float-offset"]=!1,e["flood-color"]=!1,e["flood-opacity"]=!1,e["flow-from"]=!1,e["flow-into"]=!1,e.font=!0,e["font-family"]=!0,e["font-feature-settings"]=!0,e["font-kerning"]=!0,e["font-language-override"]=!0,e["font-size"]=!0,e["font-size-adjust"]=!0,e["font-stretch"]=!0,e["font-style"]=!0,e["font-synthesis"]=!0,e["font-variant"]=!0,e["font-variant-alternates"]=!0,e["font-variant-caps"]=!0,e["font-variant-east-asian"]=!0,e["font-variant-ligatures"]=!0,e["font-variant-numeric"]=!0,e["font-variant-position"]=!0,e["font-weight"]=!0,e.grid=!1,e["grid-area"]=!1,e["grid-auto-columns"]=!1,e["grid-auto-flow"]=!1,e["grid-auto-rows"]=!1,e["grid-column"]=!1,e["grid-column-end"]=!1,e["grid-column-start"]=!1,e["grid-row"]=!1,e["grid-row-end"]=!1,e["grid-row-start"]=!1,e["grid-template"]=!1,e["grid-template-areas"]=!1,e["grid-template-columns"]=!1,e["grid-template-rows"]=!1,e["hanging-punctuation"]=!1,e.height=!0,e.hyphens=!1,e.icon=!1,e["image-orientation"]=!1,e["image-resolution"]=!1,e["ime-mode"]=!1,e["initial-letters"]=!1,e["inline-box-align"]=!1,e["justify-content"]=!1,e["justify-items"]=!1,e["justify-self"]=!1,e.left=!1,e["letter-spacing"]=!0,e["lighting-color"]=!0,e["line-box-contain"]=!1,e["line-break"]=!1,e["line-grid"]=!1,e["line-height"]=!1,e["line-snap"]=!1,e["line-stacking"]=!1,e["line-stacking-ruby"]=!1,e["line-stacking-shift"]=!1,e["line-stacking-strategy"]=!1,e["list-style"]=!0,e["list-style-image"]=!0,e["list-style-position"]=!0,e["list-style-type"]=!0,e.margin=!0,e["margin-bottom"]=!0,e["margin-left"]=!0,e["margin-right"]=!0,e["margin-top"]=!0,e["marker-offset"]=!1,e["marker-side"]=!1,e.marks=!1,e.mask=!1,e["mask-box"]=!1,e["mask-box-outset"]=!1,e["mask-box-repeat"]=!1,e["mask-box-slice"]=!1,e["mask-box-source"]=!1,e["mask-box-width"]=!1,e["mask-clip"]=!1,e["mask-image"]=!1,e["mask-origin"]=!1,e["mask-position"]=!1,e["mask-repeat"]=!1,e["mask-size"]=!1,e["mask-source-type"]=!1,e["mask-type"]=!1,e["max-height"]=!0,e["max-lines"]=!1,e["max-width"]=!0,e["min-height"]=!0,e["min-width"]=!0,e["move-to"]=!1,e["nav-down"]=!1,e["nav-index"]=!1,e["nav-left"]=!1,e["nav-right"]=!1,e["nav-up"]=!1,e["object-fit"]=!1,e["object-position"]=!1,e.opacity=!1,e.order=!1,e.orphans=!1,e.outline=!1,e["outline-color"]=!1,e["outline-offset"]=!1,e["outline-style"]=!1,e["outline-width"]=!1,e.overflow=!1,e["overflow-wrap"]=!1,e["overflow-x"]=!1,e["overflow-y"]=!1,e.padding=!0,e["padding-bottom"]=!0,e["padding-left"]=!0,e["padding-right"]=!0,e["padding-top"]=!0,e.page=!1,e["page-break-after"]=!1,e["page-break-before"]=!1,e["page-break-inside"]=!1,e["page-policy"]=!1,e.pause=!1,e["pause-after"]=!1,e["pause-before"]=!1,e.perspective=!1,e["perspective-origin"]=!1,e.pitch=!1,e["pitch-range"]=!1,e["play-during"]=!1,e.position=!1,e["presentation-level"]=!1,e.quotes=!1,e["region-fragment"]=!1,e.resize=!1,e.rest=!1,e["rest-after"]=!1,e["rest-before"]=!1,e.richness=!1,e.right=!1,e.rotation=!1,e["rotation-point"]=!1,e["ruby-align"]=!1,e["ruby-merge"]=!1,e["ruby-position"]=!1,e["shape-image-threshold"]=!1,e["shape-outside"]=!1,e["shape-margin"]=!1,e.size=!1,e.speak=!1,e["speak-as"]=!1,e["speak-header"]=!1,e["speak-numeral"]=!1,e["speak-punctuation"]=!1,e["speech-rate"]=!1,e.stress=!1,e["string-set"]=!1,e["tab-size"]=!1,e["table-layout"]=!1,e["text-align"]=!0,e["text-align-last"]=!0,e["text-combine-upright"]=!0,e["text-decoration"]=!0,e["text-decoration-color"]=!0,e["text-decoration-line"]=!0,e["text-decoration-skip"]=!0,e["text-decoration-style"]=!0,e["text-emphasis"]=!0,e["text-emphasis-color"]=!0,e["text-emphasis-position"]=!0,e["text-emphasis-style"]=!0,e["text-height"]=!0,e["text-indent"]=!0,e["text-justify"]=!0,e["text-orientation"]=!0,e["text-overflow"]=!0,e["text-shadow"]=!0,e["text-space-collapse"]=!0,e["text-transform"]=!0,e["text-underline-position"]=!0,e["text-wrap"]=!0,e.top=!1,e.transform=!1,e["transform-origin"]=!1,e["transform-style"]=!1,e.transition=!1,e["transition-delay"]=!1,e["transition-duration"]=!1,e["transition-property"]=!1,e["transition-timing-function"]=!1,e["unicode-bidi"]=!1,e["vertical-align"]=!1,e.visibility=!1,e["voice-balance"]=!1,e["voice-duration"]=!1,e["voice-family"]=!1,e["voice-pitch"]=!1,e["voice-range"]=!1,e["voice-rate"]=!1,e["voice-stress"]=!1,e["voice-volume"]=!1,e.volume=!1,e["white-space"]=!1,e.widows=!1,e.width=!0,e["will-change"]=!1,e["word-break"]=!0,e["word-spacing"]=!0,e["word-wrap"]=!0,e["wrap-flow"]=!1,e["wrap-through"]=!1,e["writing-mode"]=!1,e["z-index"]=!1,e}function em(e,t,i){}function tm(e,t,i){}var im=/javascript\s*\:/img;function rm(e,t){return im.test(t)?"":t}fr.whiteList=Zh();fr.getDefaultWhiteList=Zh;fr.onAttr=em;fr.onIgnoreAttr=tm;fr.safeAttrValue=rm;var om={indexOf:function(e,t){var i,r;if(Array.prototype.indexOf)return e.indexOf(t);for(i=0,r=e.length;i<r;i++)if(e[i]===t)return i;return-1},forEach:function(e,t,i){var r,o;if(Array.prototype.forEach)return e.forEach(t,i);for(r=0,o=e.length;r<o;r++)t.call(i,e[r],r,e)},trim:function(e){return String.prototype.trim?e.trim():e.replace(/(^\s*)|(\s*$)/g,"")},trimRight:function(e){return String.prototype.trimRight?e.trimRight():e.replace(/(\s*$)/g,"")}},vo=om;function nm(e,t){e=vo.trimRight(e),e[e.length-1]!==";"&&(e+=";");var i=e.length,r=!1,o=0,n=0,s="";function a(){if(!r){var d=vo.trim(e.slice(o,n)),p=d.indexOf(":");if(p!==-1){var f=vo.trim(d.slice(0,p)),b=vo.trim(d.slice(p+1));if(f){var v=t(o,s.length,f,b,d);v&&(s+=v+"; ")}}}o=n+1}for(;n<i;n++){var l=e[n];if(l==="/"&&e[n+1]==="*"){var c=e.indexOf("*/",n+2);if(c===-1)break;n=c+1,o=n+1,r=!1}else l==="("?r=!0:l===")"?r=!1:l===";"?r||a():l===`
`&&a()}return vo.trim(s)}var sm=nm,Dn=fr,am=sm;function Jc(e){return e==null}function lm(e){var t={};for(var i in e)t[i]=e[i];return t}function Kh(e){e=lm(e||{}),e.whiteList=e.whiteList||Dn.whiteList,e.onAttr=e.onAttr||Dn.onAttr,e.onIgnoreAttr=e.onIgnoreAttr||Dn.onIgnoreAttr,e.safeAttrValue=e.safeAttrValue||Dn.safeAttrValue,this.options=e}Kh.prototype.process=function(e){if(e=e||"",e=e.toString(),!e)return"";var t=this,i=t.options,r=i.whiteList,o=i.onAttr,n=i.onIgnoreAttr,s=i.safeAttrValue,a=am(e,function(l,c,d,p,f){var b=r[d],v=!1;if(b===!0?v=b:typeof b=="function"?v=b(p):b instanceof RegExp&&(v=b.test(p)),v!==!0&&(v=!1),p=s(d,p),!!p){var _={position:c,sourcePosition:l,source:f,isWhite:v};if(v){var k=o(d,p,_);return Jc(k)?d+":"+p:k}else{var k=n(d,p,_);if(!Jc(k))return k}}});return a};var cm=Kh;(function(e,t){var i=fr,r=cm;function o(s,a){var l=new r(a);return l.process(s)}t=e.exports=o,t.FilterCSS=r;for(var n in i)t[n]=i[n];typeof window<"u"&&(window.filterCSS=e.exports)})(Xa,Xa.exports);var Vl=Xa.exports,Nl={indexOf:function(e,t){var i,r;if(Array.prototype.indexOf)return e.indexOf(t);for(i=0,r=e.length;i<r;i++)if(e[i]===t)return i;return-1},forEach:function(e,t,i){var r,o;if(Array.prototype.forEach)return e.forEach(t,i);for(r=0,o=e.length;r<o;r++)t.call(i,e[r],r,e)},trim:function(e){return String.prototype.trim?e.trim():e.replace(/(^\s*)|(\s*$)/g,"")},spaceIndex:function(e){var t=/\s|\n|\t/,i=t.exec(e);return i?i.index:-1}},dm=Vl.FilterCSS,hm=Vl.getDefaultWhiteList,ms=Nl;function Qh(){return{a:["target","href","title"],abbr:["title"],address:[],area:["shape","coords","href","alt"],article:[],aside:[],audio:["autoplay","controls","crossorigin","loop","muted","preload","src"],b:[],bdi:["dir"],bdo:["dir"],big:[],blockquote:["cite"],br:[],caption:[],center:[],cite:[],code:[],col:["align","valign","span","width"],colgroup:["align","valign","span","width"],dd:[],del:["datetime"],details:["open"],div:[],dl:[],dt:[],em:[],figcaption:[],figure:[],font:["color","size","face"],footer:[],h1:[],h2:[],h3:[],h4:[],h5:[],h6:[],header:[],hr:[],i:[],img:["src","alt","title","width","height","loading"],ins:["datetime"],kbd:[],li:[],mark:[],nav:[],ol:[],p:[],pre:[],s:[],section:[],small:[],span:[],sub:[],summary:[],sup:[],strong:[],strike:[],table:["width","border","align","valign"],tbody:["align","valign"],td:["width","rowspan","colspan","align","valign"],tfoot:["align","valign"],th:["width","rowspan","colspan","align","valign"],thead:["align","valign"],tr:["rowspan","align","valign"],tt:[],u:[],ul:[],video:["autoplay","controls","crossorigin","loop","muted","playsinline","poster","preload","src","height","width"]}}var Jh=new dm;function pm(e,t,i){}function um(e,t,i){}function fm(e,t,i){}function gm(e,t,i){}function ep(e){return e.replace(bm,"&lt;").replace(vm,"&gt;")}function mm(e,t,i,r){if(i=sp(i),t==="href"||t==="src"){if(i=ms.trim(i),i==="#")return"#";if(!(i.substr(0,7)==="http://"||i.substr(0,8)==="https://"||i.substr(0,7)==="mailto:"||i.substr(0,4)==="tel:"||i.substr(0,11)==="data:image/"||i.substr(0,6)==="ftp://"||i.substr(0,2)==="./"||i.substr(0,3)==="../"||i[0]==="#"||i[0]==="/"))return""}else if(t==="background"){if(In.lastIndex=0,In.test(i))return""}else if(t==="style"){if(ed.lastIndex=0,ed.test(i)||(td.lastIndex=0,td.test(i)&&(In.lastIndex=0,In.test(i))))return"";r!==!1&&(r=r||Jh,i=r.process(i))}return i=ap(i),i}var bm=/</g,vm=/>/g,_m=/"/g,ym=/&quot;/g,xm=/&#([a-zA-Z0-9]*);?/gim,wm=/&colon;?/gim,$m=/&newline;?/gim,In=/((j\s*a\s*v\s*a|v\s*b|l\s*i\s*v\s*e)\s*s\s*c\s*r\s*i\s*p\s*t\s*|m\s*o\s*c\s*h\s*a):/gi,ed=/e\s*x\s*p\s*r\s*e\s*s\s*s\s*i\s*o\s*n\s*\(.*/gi,td=/u\s*r\s*l\s*\(.*/gi;function tp(e){return e.replace(_m,"&quot;")}function ip(e){return e.replace(ym,'"')}function rp(e){return e.replace(xm,function(i,r){return r[0]==="x"||r[0]==="X"?String.fromCharCode(parseInt(r.substr(1),16)):String.fromCharCode(parseInt(r,10))})}function op(e){return e.replace(wm,":").replace($m," ")}function np(e){for(var t="",i=0,r=e.length;i<r;i++)t+=e.charCodeAt(i)<32?" ":e.charAt(i);return ms.trim(t)}function sp(e){return e=ip(e),e=rp(e),e=op(e),e=np(e),e}function ap(e){return e=tp(e),e=ep(e),e}function km(){return""}function Cm(e,t){typeof t!="function"&&(t=function(){});var i=!Array.isArray(e);function r(s){return i?!0:ms.indexOf(e,s)!==-1}var o=[],n=!1;return{onIgnoreTag:function(s,a,l){if(r(s))if(l.isClosing){var c="[/removed]",d=l.position+c.length;return o.push([n!==!1?n:l.position,d]),n=!1,c}else return n||(n=l.position),"[removed]";else return t(s,a,l)},remove:function(s){var a="",l=0;return ms.forEach(o,function(c){a+=s.slice(l,c[0]),l=c[1]}),a+=s.slice(l),a}}}function Sm(e){for(var t="",i=0;i<e.length;){var r=e.indexOf("<!--",i);if(r===-1){t+=e.slice(i);break}t+=e.slice(i,r);var o=e.indexOf("-->",r);if(o===-1)break;i=o+3}return t}function Em(e){var t=e.split("");return t=t.filter(function(i){var r=i.charCodeAt(0);return r===127?!1:r<=31?r===10||r===13:!0}),t.join("")}q.whiteList=Qh();q.getDefaultWhiteList=Qh;q.onTag=pm;q.onIgnoreTag=um;q.onTagAttr=fm;q.onIgnoreTagAttr=gm;q.safeAttrValue=mm;q.escapeHtml=ep;q.escapeQuote=tp;q.unescapeQuote=ip;q.escapeHtmlEntities=rp;q.escapeDangerHtml5Entities=op;q.clearNonPrintableCharacter=np;q.friendlyAttrValue=sp;q.escapeAttrValue=ap;q.onIgnoreTagStripAll=km;q.StripTagBody=Cm;q.stripCommentTag=Sm;q.stripBlankChar=Em;q.attributeWrapSign='"';q.cssFilter=Jh;q.getDefaultCSSWhiteList=hm;var Fs={},ei=Nl;function Pm(e){var t=ei.spaceIndex(e),i;return t===-1?i=e.slice(1,-1):i=e.slice(1,t+1),i=ei.trim(i).toLowerCase(),i.slice(0,1)==="/"&&(i=i.slice(1)),i.slice(-1)==="/"&&(i=i.slice(0,-1)),i}function Am(e){return e.slice(0,2)==="</"}function Tm(e,t,i){var r="",o=0,n=!1,s=!1,a=0,l=e.length,c="",d="";e:for(a=0;a<l;a++){var p=e.charAt(a);if(n===!1){if(p==="<"){n=a;continue}}else if(s===!1){if(p==="<"){r+=i(e.slice(o,a)),n=a,o=a;continue}if(p===">"||a===l-1){r+=i(e.slice(o,n)),d=e.slice(n,a+1),c=Pm(d),r+=t(n,r.length,c,d,Am(d)),o=a+1,n=!1;continue}if(p==='"'||p==="'")for(var f=1,b=e.charAt(a-f);b.trim()===""||b==="=";){if(b==="="){s=p;continue e}b=e.charAt(a-++f)}}else if(p===s){s=!1;continue}}return o<l&&(r+=i(e.substr(o))),r}var Mm=/[^a-zA-Z0-9\\_:.-]/gim;function Om(e,t){var i=0,r=0,o=[],n=!1,s=e.length;function a(f,b){if(f=ei.trim(f),f=f.replace(Mm,"").toLowerCase(),!(f.length<1)){var v=t(f,b||"");v&&o.push(v)}}for(var l=0;l<s;l++){var c=e.charAt(l),d,p;if(n===!1&&c==="="){n=e.slice(i,l),i=l+1,r=e.charAt(i)==='"'||e.charAt(i)==="'"?i:Dm(e,l+1);continue}if(n!==!1&&l===r){if(p=e.indexOf(c,l+1),p===-1)break;d=ei.trim(e.slice(r+1,p)),a(n,d),n=!1,l=p,i=l+1;continue}if(/\s|\n|\t/.test(c))if(e=e.replace(/\s|\n|\t/g," "),n===!1)if(p=Lm(e,l),p===-1){d=ei.trim(e.slice(i,l)),a(d),n=!1,i=l+1;continue}else{l=p-1;continue}else if(p=Im(e,l-1),p===-1){d=ei.trim(e.slice(i,l)),d=id(d),a(n,d),n=!1,i=l+1;continue}else continue}return i<e.length&&(n===!1?a(e.slice(i)):a(n,id(ei.trim(e.slice(i))))),ei.trim(o.join(" "))}function Lm(e,t){for(;t<e.length;t++){var i=e[t];if(i!==" ")return i==="="?t:-1}}function Dm(e,t){for(;t<e.length;t++){var i=e[t];if(i!==" ")return i==="'"||i==='"'?t:-1}}function Im(e,t){for(;t>0;t--){var i=e[t];if(i!==" ")return i==="="?t:-1}}function Hm(e){return e[0]==='"'&&e[e.length-1]==='"'||e[0]==="'"&&e[e.length-1]==="'"}function id(e){return Hm(e)?e.substr(1,e.length-2):e}Fs.parseTag=Tm;Fs.parseAttr=Om;var zm=Vl.FilterCSS,ze=q,lp=Fs,Vm=lp.parseTag,Nm=lp.parseAttr,es=Nl;function Hn(e){return e==null}function Rm(e){var t=es.spaceIndex(e);if(t===-1)return{html:"",closing:e[e.length-2]==="/"};e=es.trim(e.slice(t+1,-1));var i=e[e.length-1]==="/";return i&&(e=es.trim(e.slice(0,-1))),{html:e,closing:i}}function Fm(e){var t={};for(var i in e)t[i]=e[i];return t}function Bm(e){var t={};for(var i in e)Array.isArray(e[i])?t[i.toLowerCase()]=e[i].map(function(r){return r.toLowerCase()}):t[i.toLowerCase()]=e[i];return t}function cp(e){e=Fm(e||{}),e.stripIgnoreTag&&(e.onIgnoreTag&&console.error('Notes: cannot use these two options "stripIgnoreTag" and "onIgnoreTag" at the same time'),e.onIgnoreTag=ze.onIgnoreTagStripAll),e.whiteList||e.allowList?e.whiteList=Bm(e.whiteList||e.allowList):e.whiteList=ze.whiteList,this.attributeWrapSign=e.singleQuotedAttributeValue===!0?"'":ze.attributeWrapSign,e.onTag=e.onTag||ze.onTag,e.onTagAttr=e.onTagAttr||ze.onTagAttr,e.onIgnoreTag=e.onIgnoreTag||ze.onIgnoreTag,e.onIgnoreTagAttr=e.onIgnoreTagAttr||ze.onIgnoreTagAttr,e.safeAttrValue=e.safeAttrValue||ze.safeAttrValue,e.escapeHtml=e.escapeHtml||ze.escapeHtml,this.options=e,e.css===!1?this.cssFilter=!1:(e.css=e.css||{},this.cssFilter=new zm(e.css))}cp.prototype.process=function(e){if(e=e||"",e=e.toString(),!e)return"";var t=this,i=t.options,r=i.whiteList,o=i.onTag,n=i.onIgnoreTag,s=i.onTagAttr,a=i.onIgnoreTagAttr,l=i.safeAttrValue,c=i.escapeHtml,d=t.attributeWrapSign,p=t.cssFilter;i.stripBlankChar&&(e=ze.stripBlankChar(e)),i.allowCommentTag||(e=ze.stripCommentTag(e));var f=!1;i.stripIgnoreTagBody&&(f=ze.StripTagBody(i.stripIgnoreTagBody,n),n=f.onIgnoreTag);var b=Vm(e,function(v,_,k,C,O){var I={sourcePosition:v,position:_,isClosing:O,isWhite:Object.prototype.hasOwnProperty.call(r,k)},V=o(k,C,I);if(!Hn(V))return V;if(I.isWhite){if(I.isClosing)return"</"+k+">";var K=Rm(C),R=r[k],Q=Nm(K.html,function(de,G){var Ie=es.indexOf(R,de)!==-1,_e=s(k,de,G,Ie);return Hn(_e)?Ie?(G=l(k,de,G,p),G?de+"="+d+G+d:de):(_e=a(k,de,G,Ie),Hn(_e)?void 0:_e):_e});return C="<"+k,Q&&(C+=" "+Q),K.closing&&(C+=" /"),C+=">",C}else return V=n(k,C,I),Hn(V)?c(C):V},c);return f&&(b=f.remove(b)),b};var Um=cp;(function(e,t){var i=q,r=Fs,o=Um;function n(a,l){var c=new o(l);return c.process(a)}t=e.exports=n,t.filterXSS=n,t.FilterXSS=o,function(){for(var a in i)t[a]=i[a];for(var l in r)t[l]=r[l]}(),typeof window<"u"&&(window.filterXSS=e.exports);function s(){return typeof self<"u"&&typeof DedicatedWorkerGlobalScope<"u"&&self instanceof DedicatedWorkerGlobalScope}s()&&(self.filterXSS=e.exports)})(Ga,Ga.exports);var da=Ga.exports;let ha;function jm(e,t={},i={}){ha||(ha={...da.getDefaultWhiteList(),table:[...da.getDefaultWhiteList().table??[],"role"],input:["type","disabled","checked"],"ha-icon":["icon"],"ha-svg-icon":["path"],"ha-alert":["alert-type","title"]});const r={...ha};i.allowDataUrl&&r.a&&(r.a=[...r.a,"download"]),H.setOptions({gfm:!0,breaks:!!t.breaks,...t}),H.use({renderer:{table(...n){return`<div>${new H.Renderer().table.apply(this,n)}</div>`}}});const o=H.parse(e,{async:!1});return da.filterXSS(o,{whiteList:r,onTagAttr:(n,s,a)=>{if(n==="input")return s==="type"&&a==="checkbox"||s==="checked"||s==="disabled"?void 0:"";if(i.allowDataUrl&&n==="a"&&s==="href"&&a.startsWith("data:"))return`href="${a}"`}})}var qm=Object.defineProperty,Gm=Object.getOwnPropertyDescriptor,Bs=(e,t,i,r)=>{for(var o=r>1?void 0:r?Gm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&qm(t,i,o),o};let Xo=class extends Di{constructor(){super(...arguments),this.allowSvg=!1,this.breaks=!1}createRenderRoot(){return this}update(e){super.update(e),this.content!==void 0&&e.has("content")?this._renderMarkdown():(e.has("breaks")||e.has("allowSvg"))&&this._renderMarkdown()}_renderMarkdown(){const e=jm(String(this.content??""),{breaks:this.breaks,gfm:!0},{allowSvg:this.allowSvg});this.innerHTML=e,this.querySelectorAll("a").forEach(t=>{try{t.host&&t.host!==document.location.host&&(t.target="_blank",t.rel="noreferrer noopener")}catch{}})}};Bs([u()],Xo.prototype,"content",2);Bs([u({attribute:"allow-svg",type:Boolean})],Xo.prototype,"allowSvg",2);Bs([u({type:Boolean})],Xo.prototype,"breaks",2);Xo=Bs([x("ha-markdown-element")],Xo);var Xm=Object.defineProperty,Wm=Object.getOwnPropertyDescriptor,Us=(e,t,i,r)=>{for(var o=r>1?void 0:r?Wm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Xm(t,i,o),o};let Hr=class extends y{constructor(){super(...arguments),this.allowSvg=!1,this.breaks=!1}render(){return this.content?h`
      <ha-markdown-element
        .content=${this.content}
        .allowSvg=${this.allowSvg}
        .breaks=${this.breaks}
      ></ha-markdown-element>
    `:m}};Hr.styles=w`
    :host {
      display: block;
    }
    ha-markdown-element {
      -ms-user-select: text;
      -webkit-user-select: text;
      -moz-user-select: text;
      user-select: text;
    }
    a {
      color: var(--markdown-link-color, var(--primary-color));
    }
    img {
      max-width: 100%;
    }
    code,
    pre {
      background-color: var(--markdown-code-background-color, rgba(0, 0, 0, 0.06));
      border-radius: var(--ha-border-radius-sm, 4px);
      color: var(--markdown-code-text-color, inherit);
    }
    code {
      font-size: var(--ha-font-size-s, 12px);
      padding: 0.2em 0.4em;
    }
    pre {
      padding: var(--ha-space-4, 16px);
      overflow: auto;
      line-height: var(--ha-line-height-condensed, 1.25);
      font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
    }
    pre code {
      padding: 0;
      background: transparent;
    }
    h1,
    h2,
    h3,
    h4,
    h5,
    h6 {
      line-height: initial;
      margin: 0.6em 0 0.35em;
    }
    h2 {
      font-size: var(--ha-font-size-xl, 22px);
      font-weight: var(--ha-font-weight-bold, 700);
    }
    p {
      margin: 0.5em 0;
    }
    hr {
      border: none;
      border-top: 1px solid var(--divider-color);
      margin: var(--ha-space-4, 16px) 0;
    }
    table {
      border-collapse: collapse;
      width: 100%;
      margin: 0.5em 0;
    }
    td,
    th {
      border: 1px solid var(--divider-color);
      padding: 0.25em 0.5em;
      text-align: start;
    }
    blockquote {
      border-left: 4px solid var(--divider-color);
      margin-inline: 0;
      padding-inline: 1em;
      color: var(--secondary-text-color);
    }
    ul,
    ol {
      padding-inline-start: 1.5em;
      margin: 0.5em 0;
    }
    li:has(input[type="checkbox"]) {
      list-style: none;
    }
  `;Us([u()],Hr.prototype,"content",2);Us([u({attribute:"allow-svg",type:Boolean})],Hr.prototype,"allowSvg",2);Us([u({type:Boolean})],Hr.prototype,"breaks",2);Hr=Us([x("ha-markdown")],Hr);var Ym=Object.defineProperty,Zm=Object.getOwnPropertyDescriptor,Rl=(e,t,i,r)=>{for(var o=r>1?void 0:r?Zm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ym(t,i,o),o};let zr=class extends y{static getStubConfig(){return{type:"markdown",content:`The **Markdown** card allows you to write any text. You can style it **bold**, *italicized*, ~~strikethrough~~ etc. You can do images, links, and more.

For more information see the [Markdown Cheatsheet](https://commonmark.org/help).`}}setConfig(e){if(!e.content&&e.content!=="")throw new Error("Content required");this._config=e}getGridOptions(){return{columns:"full",rows:"auto",min_columns:12,min_rows:1}}render(){if(!this._config)return m;const e=!!this._config.text_only,t=String(this._config.title??""),i=String(this._config.content??"");return h`
      <ha-card
        class=${T({"text-only":e,"with-header":!!t&&!e})}
      >
        ${t&&!e?h`<h1 class="card-header">${t}</h1>`:m}
        <ha-markdown
          breaks
          .content=${i}
        ></ha-markdown>
      </ha-card>
    `}};zr.styles=w`
    ha-card {
      height: 100%;
      overflow-y: auto;
    }
    .card-header {
      margin: 0;
      padding: 16px 16px 0;
      font-size: 16px;
      font-weight: 600;
      color: var(--primary-text-color);
      line-height: 1.3;
    }
    ha-markdown {
      display: block;
      padding: 16px;
      word-wrap: break-word;
      overflow-wrap: anywhere;
    }
    .with-header ha-markdown {
      padding-top: 8px;
    }
    .text-only {
      background: none;
      box-shadow: none;
      border: none;
    }
    .text-only ha-markdown {
      padding: 2px 4px;
    }
  `;Rl([u({attribute:!1})],zr.prototype,"flow",2);Rl([g()],zr.prototype,"_config",2);zr=Rl([x("hui-markdown-card")],zr);const me=w`
  ha-card {
    height: 100%;
    transition:
      box-shadow 180ms ease-in-out,
      border-color 180ms ease-in-out;
  }

  ha-card:hover {
    box-shadow:
      var(--ha-card-box-shadow),
      0 2px 6px rgba(0, 0, 0, 0.06);
  }

  ha-tile-icon {
    --tile-icon-color: var(--tile-color);
  }
`,be=w`
  :host {
    --tile-color: var(--state-inactive-color);
  }

  ha-card.active {
    --tile-color: var(--state-icon-color);
  }
`;var Km=Object.defineProperty,Qm=Object.getOwnPropertyDescriptor,vn=(e,t,i,r)=>{for(var o=r>1?void 0:r?Qm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Km(t,i,o),o};let Wi=class extends y{constructor(){super(...arguments),this.vertical=!1,this.fixedInfoHeight=!1,this.expandFeatures=!1,this._hasFeatures=!1}_handleFeaturesSlotChange(e){this._hasFeatures=e.target.assignedElements().length>0}render(){const e={content:!0,vertical:this.vertical,"fixed-info-height":this.fixedInfoHeight,"has-features":this._hasFeatures};return h`
      <div class="container">
        <div class="row">
          <div class=${T(e)}>
            <slot name="icon"></slot>
            <slot name="info"></slot>
          </div>
        </div>
        <slot name="features" @slotchange=${this._handleFeaturesSlotChange}></slot>
      </div>
    `}};Wi.styles=w`
    :host {
      --ha-ripple-color: var(--tile-color);
      height: 100%;
      width: 100%;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      min-height: 0;
    }
    .container {
      display: flex;
      flex-direction: column;
      flex: 1;
      min-height: 0;
      margin: calc(-1 * var(--ha-card-border-width, 1px));
    }
    /* HA: column flex so .content can grow and vertically center */
    .row {
      display: flex;
      flex-direction: column;
      flex: 1;
      min-height: 0;
      min-width: 0;
    }
    :host([expand-features]) .row {
      flex: 0 0 auto;
      min-height: auto;
    }
    .content {
      position: relative;
      display: flex;
      flex-direction: row;
      align-items: center;
      padding: 0 10px;
      min-height: var(--row-height, 56px);
      flex: 1;
      min-width: 0;
      gap: 10px;
      box-sizing: border-box;
    }
    :host([expand-features]) .content {
      flex: 0 0 auto;
    }
    :host([expand-features]) .content.has-features:not(.vertical) {
      max-height: none;
    }
    .content.has-features:not(.vertical) {
      max-height: var(--row-height, 56px);
    }
    .content.vertical {
      flex-direction: column;
      text-align: center;
      justify-content: center;
      padding: 10px var(--ha-space-2, 8px);
      gap: 8px;
      max-height: none;
      --ha-tile-info-align-items: center;
      --ha-tile-info-text-align: center;
    }
    .content.vertical.fixed-info-height {
      gap: 2px;
      --ha-tile-info-gap: 2px;
      --ha-tile-info-primary-line-height: var(--ha-space-4, 16px);
      --ha-tile-info-primary-min-height: var(--ha-space-8, 32px);
      --ha-tile-info-min-height: var(--ha-space-12, 48px);
    }
    .content.vertical ::slotted([slot="info"]) {
      width: 100%;
      flex: none;
    }
    ::slotted([slot="icon"]) {
      flex-shrink: 0;
      position: relative;
      padding: 6px;
      margin: -6px;
    }
    ::slotted([slot="info"]) {
      position: relative;
      min-width: 0;
      flex: 1;
      box-sizing: border-box;
    }
    ::slotted([slot="features"]) {
      display: block;
      flex-shrink: 0;
      padding: 0 var(--ha-space-3, 12px) var(--ha-space-3, 12px);
      box-sizing: border-box;
    }
    :host([expand-features]) ::slotted([slot="features"]) {
      flex: 1 1 auto;
      flex-shrink: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
    }
  `;vn([u({type:Boolean})],Wi.prototype,"vertical",2);vn([u({type:Boolean,attribute:"fixed-info-height"})],Wi.prototype,"fixedInfoHeight",2);vn([u({type:Boolean,attribute:"expand-features",reflect:!0})],Wi.prototype,"expandFeatures",2);vn([g()],Wi.prototype,"_hasFeatures",2);Wi=vn([x("ha-tile-container")],Wi);var Jm=Object.defineProperty,eb=Object.getOwnPropertyDescriptor,_n=(e,t,i,r)=>{for(var o=r>1?void 0:r?eb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Jm(t,i,o),o};let Yi=class extends y{constructor(){super(...arguments),this.icon="",this.interactive=!1,this.active=!1,this.large=!1}render(){return h`
      <div
        class="container background ${this.interactive?"button":""}"
        role=${this.interactive?"button":m}
        tabindex=${this.interactive?"0":m}
      >
        <ha-icon .icon=${this.icon}></ha-icon>
        <slot></slot>
      </div>
    `}};Yi.styles=w`
    :host {
      --tile-icon-color: var(--state-inactive-color);
      --tile-icon-opacity: 0.2;
      --tile-icon-hover-opacity: 0.35;
      --tile-icon-border-radius: var(
        --ha-tile-icon-border-radius,
        var(--ha-border-radius-pill, 999px)
      );
      --tile-icon-size: 36px;
      --mdc-icon-size: 24px;
      position: relative;
      display: block;
      user-select: none;
      pointer-events: none;
      flex-shrink: 0;
      transition: transform 180ms ease-in-out;
    }
    :host([interactive]) {
      -webkit-tap-highlight-color: transparent;
      pointer-events: auto;
      cursor: pointer;
    }
    :host([interactive]:active) .container {
      transform: scale(1.08);
    }
    :host([interactive]:hover) {
      --tile-icon-opacity: var(--tile-icon-hover-opacity);
    }
    :host([large]) {
      --tile-icon-size: 56px;
      --mdc-icon-size: 32px;
    }
    .container {
      position: relative;
      display: flex;
      align-items: center;
      justify-content: center;
      width: var(--tile-icon-size);
      height: var(--tile-icon-size);
      border-radius: var(--tile-icon-border-radius);
      overflow: hidden;
      box-sizing: border-box;
      transition: box-shadow 180ms ease-in-out, transform 180ms ease-in-out;
    }
    .container.button {
      pointer-events: auto;
      cursor: pointer;
    }
    .container.button:focus-visible {
      outline: none;
      box-shadow: 0 0 0 2px var(--tile-icon-color);
    }
    .container.background::before {
      content: "";
      position: absolute;
      inset: 0;
      background-color: var(--tile-icon-color);
      opacity: var(--tile-icon-opacity);
      transition:
        background-color 180ms ease-in-out,
        opacity 180ms ease-in-out;
    }
    ha-icon {
      position: relative;
      z-index: 1;
      display: flex;
      align-items: center;
      justify-content: center;
      width: var(--mdc-icon-size);
      height: var(--mdc-icon-size);
      color: var(--tile-icon-foreground, var(--tile-icon-color));
      pointer-events: none;
      transition: color 180ms ease-in-out;
    }
  `;_n([u()],Yi.prototype,"icon",2);_n([u({type:Boolean,reflect:!0})],Yi.prototype,"interactive",2);_n([u({type:Boolean})],Yi.prototype,"active",2);_n([u({type:Boolean})],Yi.prototype,"large",2);Yi=_n([x("ha-tile-icon")],Yi);var tb=Object.defineProperty,ib=Object.getOwnPropertyDescriptor,Fl=(e,t,i,r)=>{for(var o=r>1?void 0:r?ib(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&tb(t,i,o),o};let Wo=class extends y{constructor(){super(...arguments),this.primary="",this.secondary=""}render(){return h`
      <div class="info">
        <div class="primary">
          <span>${this.primary}</span>
        </div>
        ${this.secondary?h`
              <div class="secondary">
                <span>${this.secondary}</span>
              </div>
            `:""}
      </div>
    `}};Wo.styles=w`
    :host {
      display: block;
      width: 100%;
      min-width: 0;
    }
    .info {
      width: 100%;
      display: flex;
      flex-direction: column;
      align-items: var(--ha-tile-info-align-items, flex-start);
      justify-content: center;
      gap: var(--ha-tile-info-gap, 0);
      min-width: 0;
      min-height: var(--ha-tile-info-min-height, auto);
      text-align: var(--ha-tile-info-text-align, start);
    }
    .primary span,
    .secondary span {
      text-overflow: ellipsis;
      overflow: hidden;
      white-space: nowrap;
      width: 100%;
      display: block;
    }
    .primary {
      width: 100%;
      font-size: var(--ha-font-size-m, 14px);
      font-weight: var(--ha-font-weight-medium, 500);
      line-height: var(--ha-tile-info-primary-line-height, var(--ha-line-height-normal, 1.4));
      min-height: var(--ha-tile-info-primary-min-height, auto);
      letter-spacing: 0.1px;
      color: var(--primary-text-color);
    }
    .secondary {
      width: 100%;
      font-size: var(--ha-font-size-s, 12px);
      font-weight: var(--ha-font-weight-normal, 400);
      line-height: var(--ha-line-height-condensed, 1.25);
      letter-spacing: 0.4px;
      color: var(--ha-tile-info-secondary-color, var(--primary-text-color));
    }
  `;Fl([u()],Wo.prototype,"primary",2);Fl([u()],Wo.prototype,"secondary",2);Wo=Fl([x("ha-tile-info")],Wo);var rb=Object.defineProperty,ob=Object.getOwnPropertyDescriptor,Bl=(e,t,i,r)=>{for(var o=r>1?void 0:r?ob(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&rb(t,i,o),o};let Vr=class extends y{static getStubConfig(){return{type:"sensor",entity:"/demo/temperature",name:"Temperature",icon:"mdi:thermometer",unit:"°C"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}render(){var p,f,b;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,i=String(this._config.name??((p=t==null?void 0:t.attributes)==null?void 0:p.friendly_name)??e??"Sensor"),r=String(this._config.icon??((f=t==null?void 0:t.attributes)==null?void 0:f.icon)??"mdi:eye"),o=String(this._config.unit??((b=t==null?void 0:t.attributes)==null?void 0:b.unit_of_measurement)??"").trim(),n=!!this._config.hide_state,s=!!this._config.vertical,a=t==null?void 0:t.state,l=n?"":a==null?"—":o?`${a} ${o}`:String(a),c=P({"--tile-color":El(this._config.color)}),d=s?P({"--ha-tile-info-gap":"4px","--ha-tile-info-min-height":"auto","--ha-tile-info-primary-min-height":"auto","--ha-tile-info-primary-line-height":"1.3","--ha-tile-info-align-items":"center","--ha-tile-info-text-align":"center"}):m;return h`
      <ha-card style=${c}>
        <ha-tile-container ?vertical=${s} ?fixed-info-height=${s} style=${d}>
          <ha-tile-icon slot="icon" .icon=${r}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${i} .secondary=${l}></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `}};Vr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
    `];Bl([u({attribute:!1})],Vr.prototype,"flow",2);Bl([g()],Vr.prototype,"_config",2);Vr=Bl([x("hui-sensor-card")],Vr);var nb=Object.defineProperty,sb=Object.getOwnPropertyDescriptor,Ul=(e,t,i,r)=>{for(var o=r>1?void 0:r?sb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&nb(t,i,o),o};function ab(e){if(e==null)return"—";if(typeof e=="string")return e.trim()?e:"—";if(typeof e=="boolean")return e?"是":"否";if(typeof e=="number")return String(e);if(Array.isArray(e))return e.map(t=>String(t)).join(", ")||"—";try{return JSON.stringify(e)}catch{return String(e)}}let Nr=class extends y{static getStubConfig(){return{type:"label",entity:"/demo/string",name:"当前素材",icon:"mdi:label-outline"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}render(){var c,d;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,i=String(this._config.name??((c=t==null?void 0:t.attributes)==null?void 0:c.friendly_name)??e??"标签"),r=String(this._config.icon??((d=t==null?void 0:t.attributes)==null?void 0:d.icon)??"mdi:label-outline"),o=!!this._config.hide_state,n=!!this._config.vertical,s=o?"":ab(t==null?void 0:t.state),a=P({"--tile-color":El(this._config.color)}),l=n?P({"--ha-tile-info-gap":"4px","--ha-tile-info-min-height":"auto","--ha-tile-info-primary-min-height":"auto","--ha-tile-info-primary-line-height":"1.3","--ha-tile-info-align-items":"center","--ha-tile-info-text-align":"center"}):m;return h`
      <ha-card style=${a}>
        <ha-tile-container ?vertical=${n} ?fixed-info-height=${n} style=${l}>
          <ha-tile-icon slot="icon" .icon=${r}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${i} .secondary=${s}></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `}};Nr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
    `];Ul([u({attribute:!1})],Nr.prototype,"flow",2);Ul([g()],Nr.prototype,"_config",2);Nr=Ul([x("hui-label-card")],Nr);var lb=Object.defineProperty,cb=Object.getOwnPropertyDescriptor,st=(e,t,i,r)=>{for(var o=r>1?void 0:r?cb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&lb(t,i,o),o};let Te=class extends y{constructor(){super(...arguments),this.min=0,this.max=100,this.step=1,this.disabled=!1,this.showHandle=!0,this.roundValue=!0,this.vertical=!1,this._pressed=!1,this._onPointerDown=e=>{var t;this.disabled||(e.preventDefault(),this._pressed=!0,this._pointerId=e.pointerId,(t=this._slider)==null||t.setPointerCapture(e.pointerId),this._setFromPointer(e.clientX,e.clientY))},this._onPointerMove=e=>{!this._pressed||e.pointerId!==this._pointerId||this._setFromPointer(e.clientX,e.clientY)},this._onPointerUp=e=>{var t;if(e.pointerId===this._pointerId){this._pressed=!1,this._pointerId=void 0;try{(t=this._slider)==null||t.releasePointerCapture(e.pointerId)}catch{}}},this._onKeyDown=e=>{if(this.disabled)return;let t=this.value??this.min;switch(e.key){case"ArrowRight":case"ArrowUp":t+=this.step;break;case"ArrowLeft":case"ArrowDown":t-=this.step;break;case"Home":t=this.min;break;case"End":t=this.max;break;default:return}e.preventDefault(),t=this.steppedValue(t),this.value=t,this._emitChange(t)}}boundedValue(e){return Math.min(Math.max(e,this.min),this.max)}steppedValue(e){return this.boundedValue(Math.round(e/this.step)*this.step)}percentage(){const e=this.value??this.min;return this.max===this.min?0:(this.boundedValue(e)-this.min)/(this.max-this.min)}_emitChange(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}_setFromPointer(e,t){const i=this._slider;if(!i)return;const r=i.getBoundingClientRect();let o;this.vertical?o=Math.max(0,Math.min(1,(r.bottom-t)/r.height)):o=Math.max(0,Math.min(1,(e-r.left)/r.width));const n=this.steppedValue(this.min+o*(this.max-this.min));this.value=n,this._emitChange(n)}render(){const e=this.value??this.min,t=this.roundValue?Math.round(e):e;return h`
      <div class="container ${this._pressed?"pressed":""}">
        <div
          class="slider"
          tabindex=${this.disabled?m:"0"}
          role="slider"
          aria-orientation=${this.vertical?"vertical":"horizontal"}
          aria-valuemin=${String(this.min)}
          aria-valuemax=${String(this.max)}
          aria-valuenow=${String(t)}
          aria-disabled=${this.disabled?"true":"false"}
          @pointerdown=${this._onPointerDown}
          @pointermove=${this._onPointerMove}
          @pointerup=${this._onPointerUp}
          @pointercancel=${this._onPointerUp}
          @keydown=${this._onKeyDown}
        >
          <div class="slider-track-background"></div>
          <div
            class="slider-track-bar ${this.showHandle?"show-handle":""}"
            style=${P({"--value":String(this.percentage())})}
          ></div>
        </div>
      </div>
    `}};Te.styles=w`
    :host {
      display: block;
      --control-slider-color: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-slider-background: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-slider-background-opacity: 0.2;
      --control-slider-thickness: var(--feature-height, 40px);
      --control-slider-border-radius: var(
        --feature-border-radius,
        var(--ha-card-border-radius, var(--ha-border-radius-lg, 12px))
      );
      height: var(--control-slider-thickness);
      width: 100%;
    }
    :host([vertical]) {
      height: 100%;
      width: var(--control-slider-thickness);
    }
    .container {
      position: relative;
      height: 100%;
      width: 100%;
      --handle-size: 4px;
      --handle-margin: calc(var(--control-slider-thickness) / 8);
    }
    .slider {
      position: relative;
      height: 100%;
      width: 100%;
      border-radius: var(--control-slider-border-radius);
      outline: none;
      overflow: hidden;
      cursor: pointer;
      touch-action: none;
    }
    .slider:focus-visible {
      box-shadow: 0 0 0 2px var(--control-slider-color);
    }
    .slider * {
      pointer-events: none;
    }
    .slider-track-background {
      position: absolute;
      inset: 0;
      background: var(--control-slider-background);
      opacity: var(--control-slider-background-opacity);
    }
    .slider-track-bar {
      --ha-border-radius: var(--control-slider-border-radius);
      --slider-size: 100%;
      position: absolute;
      top: 0;
      left: 0;
      height: 100%;
      width: 100%;
      background-color: var(--control-slider-color);
      transform: translate3d(calc((var(--value, 0) - 1) * var(--slider-size)), 0, 0);
      border-radius: var(--control-slider-border-radius);
      transition:
        transform 180ms ease-in-out,
        background-color 180ms ease-in-out;
    }
    :host([vertical]) .slider-track-bar {
      transform: translate3d(0, calc((1 - var(--value, 0)) * var(--slider-size)), 0);
    }
    .slider-track-bar.show-handle {
      --slider-size: calc(100% - 2 * var(--handle-margin) - var(--handle-size));
    }
    .slider-track-bar::after {
      display: block;
      content: "";
      position: absolute;
      top: 0;
      bottom: 0;
      right: var(--handle-margin);
      margin: auto;
      height: 50%;
      width: var(--handle-size);
      border-radius: var(--handle-size);
      background-color: white;
    }
    :host([vertical]) .slider-track-bar::after {
      top: var(--handle-margin);
      bottom: auto;
      right: 0;
      left: 0;
      margin: auto;
      width: 50%;
      height: var(--handle-size);
    }
    .pressed .slider-track-bar {
      transition: none;
    }
    :host([disabled]) .slider {
      cursor: not-allowed;
      opacity: 0.5;
    }
  `;st([u({type:Number})],Te.prototype,"value",2);st([u({type:Number})],Te.prototype,"min",2);st([u({type:Number})],Te.prototype,"max",2);st([u({type:Number})],Te.prototype,"step",2);st([u({type:Boolean,reflect:!0})],Te.prototype,"disabled",2);st([u({type:Boolean,attribute:"show-handle"})],Te.prototype,"showHandle",2);st([u({type:Boolean,attribute:"round-value"})],Te.prototype,"roundValue",2);st([u({type:Boolean,reflect:!0})],Te.prototype,"vertical",2);st([g()],Te.prototype,"_pressed",2);st([mi(".slider")],Te.prototype,"_slider",2);Te=st([x("ha-control-slider")],Te);var db=Object.defineProperty,hb=Object.getOwnPropertyDescriptor,jl=(e,t,i,r)=>{for(var o=r>1?void 0:r?hb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&db(t,i,o),o};let Rr=class extends y{static getStubConfig(){return{type:"slider",entity:"/demo/spotlights",name:"Spotlights",icon:"mdi:spotlight-beam",min:0,max:100,step:1}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_value(){var i,r;const e=(i=this._config)==null?void 0:i.entity;if(!e||!this.flow)return 0;const t=(r=this.flow.states[e])==null?void 0:r.state;return typeof t=="number"?t:Number(t)||0}_percentFractionDigits(e,t,i){if(t-e===0||i<=0)return 2;const r=Math.abs(i)/Math.abs(t-e)*100;if(r<=0)return 2;const o=Math.max(0,Math.ceil(-Math.log10(r)));return Math.min(o,6)}_formatValue(e){var l,c,d;const t=Number(((l=this._config)==null?void 0:l.max)??100),i=Number(((c=this._config)==null?void 0:c.min)??0),r=Number(((d=this._config)==null?void 0:d.step)??1),o=Math.min(Math.max(e,i),t),n=t-i===0?0:(o-i)/(t-i)*100,s=this._percentFractionDigits(i,t,r);return`${n.toFixed(s).replace(/\.0+$/,"").replace(/(\.\d*?)0+$/,"$1")}%`}async _onSliderChange(e){var i;e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,e.detail.value)}render(){var v,_;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,i=Number(this._config.min??0),r=Number(this._config.max??100),o=Number(this._config.step??1),n=this._value(),s=$e(t)||n>i,a=!!this._config.vertical,l=String(this._config.name??((v=t==null?void 0:t.attributes)==null?void 0:v.friendly_name)??e??"Slider"),c=String(this._config.icon??((_=t==null?void 0:t.attributes)==null?void 0:_.icon)??"mdi:spotlight-beam"),d=this._formatValue(n),p=Gf(s,this._config.color),f=P({"--tile-color":p}),b=P({"--feature-color":p??(s?"var(--state-icon-color)":"var(--state-inactive-color)")});return h`
      <ha-card class=${T({active:s})} style=${f}>
        <ha-tile-container ?vertical=${a} ?fixed-info-height=${a}>
          <ha-tile-icon slot="icon" .icon=${c} .active=${s}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${l} .secondary=${d}></ha-tile-info>
          <div slot="features" class="feature" style=${b}>
            <ha-control-slider
              .value=${n}
              .min=${i}
              .max=${r}
              .step=${o}
              show-handle
              round-value
              @value-changed=${this._onSliderChange}
              @click=${k=>k.stopPropagation()}
            ></ha-control-slider>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Rr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        --feature-color: var(--tile-color);
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
        --feature-color: var(--tile-color);
      }
    `];jl([u({attribute:!1})],Rr.prototype,"flow",2);jl([g()],Rr.prototype,"_config",2);Rr=jl([x("hui-slider-card")],Rr);var pb=Object.defineProperty,ub=Object.getOwnPropertyDescriptor,so=(e,t,i,r)=>{for(var o=r>1?void 0:r?ub(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&pb(t,i,o),o};let hi=class extends y{constructor(){super(...arguments),this.checked=!1,this.disabled=!1,this.vertical=!1,this.iconOn="mdi:lightbulb",this.iconOff="mdi:lightbulb-outline"}_toggle(){this.disabled||(this.checked=!this.checked,this.dispatchEvent(new Event("change",{bubbles:!0,composed:!0})))}_onKeyDown(e){(e.key==="Enter"||e.key===" ")&&(e.preventDefault(),this._toggle())}render(){return h`
      <div
        id="switch"
        class="switch"
        role="switch"
        tabindex=${this.disabled?m:"0"}
        aria-checked=${this.checked?"true":"false"}
        aria-disabled=${this.disabled?"true":"false"}
        ?disabled=${this.disabled}
        @click=${this._toggle}
        @keydown=${this._onKeyDown}
      >
        <div class="background"></div>
        <div class="button">
          <ha-icon .icon=${this.checked?this.iconOn:this.iconOff}></ha-icon>
        </div>
      </div>
    `}};hi.styles=w`
    :host {
      display: block;
      --control-switch-on-color: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-switch-off-color: var(--feature-color, var(--tile-color, var(--state-inactive-color)));
      --control-switch-background-opacity: 0.2;
      --control-switch-hover-background-opacity: 0.35;
      --control-switch-thickness: var(--feature-height, 40px);
      --control-switch-border-radius: var(
        --feature-border-radius,
        var(--ha-card-border-radius, 12px)
      );
      --control-switch-padding: 0;
      --mdc-icon-size: 20px;
      height: var(--control-switch-thickness);
      width: 100%;
      box-sizing: border-box;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }
    .switch {
      box-sizing: border-box;
      position: relative;
      height: 100%;
      width: 100%;
      border-radius: var(--control-switch-border-radius);
      outline: none;
      padding: var(--control-switch-padding);
      display: flex;
      cursor: pointer;
      overflow: hidden;
    }
    .switch:focus-visible {
      box-shadow: 0 0 0 2px var(--control-switch-off-color);
    }
    .switch[aria-checked="true"]:focus-visible {
      box-shadow: 0 0 0 2px var(--control-switch-on-color);
    }
    .switch[disabled] {
      opacity: 0.5;
      cursor: not-allowed;
    }
    .background {
      position: absolute;
      inset: 0;
      border-radius: inherit;
      background-color: var(--control-switch-off-color);
      opacity: var(--control-switch-background-opacity);
      transition: background-color 180ms ease-in-out, opacity 180ms ease-in-out;
    }
    .switch:not([disabled]):hover .background {
      opacity: var(--control-switch-hover-background-opacity);
    }
    .switch[aria-checked="true"] .background {
      background-color: var(--control-switch-on-color);
    }
    .button {
      position: relative;
      z-index: 1;
      width: 50%;
      height: 100%;
      border-radius: calc(
        var(--control-switch-border-radius) - var(--control-switch-padding)
      );
      background-color: var(--control-switch-off-color);
      color: white;
      display: flex;
      align-items: center;
      justify-content: center;
      transition:
        transform 180ms ease-in-out,
        background-color 180ms ease-in-out;
    }
    .switch[aria-checked="true"] .button {
      transform: translateX(100%);
      background-color: var(--control-switch-on-color);
    }
    :host([vertical]) .switch[aria-checked="true"] .button {
      transform: translateY(100%);
    }
    :host([vertical]) {
      width: var(--control-switch-thickness);
      height: 100%;
    }
    :host([vertical]) .button {
      width: 100%;
      height: 50%;
    }
  `;so([u({type:Boolean})],hi.prototype,"checked",2);so([u({type:Boolean,reflect:!0})],hi.prototype,"disabled",2);so([u({type:Boolean,reflect:!0})],hi.prototype,"vertical",2);so([u({attribute:"icon-on"})],hi.prototype,"iconOn",2);so([u({attribute:"icon-off"})],hi.prototype,"iconOff",2);hi=so([x("ha-control-switch")],hi);var fb=Object.defineProperty,gb=Object.getOwnPropertyDescriptor,ql=(e,t,i,r)=>{for(var o=r>1?void 0:r?gb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&fb(t,i,o),o};let Fr=class extends y{static getStubConfig(){return{type:"switch",entity:"/demo/floor_lamp",name:"Flood light",icon:"mdi:lightbulb"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _toggle(e){var i;e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,Cl(this._entityState()))}render(){var d,p;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=$e(t),r=!!this._config.vertical,o=String(this._config.name??((d=t==null?void 0:t.attributes)==null?void 0:d.friendly_name)??e??"Switch"),n=String(this._config.icon??((p=t==null?void 0:t.attributes)==null?void 0:p.icon)??"mdi:lightbulb"),s=e&&this.flow?this.flow.formatState(e):i?"开启":"关闭",a=Xf(i,this._config.color),l=P({"--tile-color":a}),c=P({"--feature-color":a??(i?"var(--state-icon-color)":"var(--state-inactive-color)")});return h`
      <ha-card class=${T({active:i})} style=${l}>
        <ha-tile-container ?vertical=${r} ?fixed-info-height=${r}>
          <ha-tile-icon slot="icon" .icon=${n} .active=${i}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${o} .secondary=${s}></ha-tile-info>
          <div slot="features" class="feature" style=${c}>
            <ha-control-switch
              .checked=${i}
              icon-on="mdi:lightbulb"
              icon-off="mdi:lightbulb-outline"
              @change=${this._toggle}
              @click=${f=>f.stopPropagation()}
            ></ha-control-switch>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Fr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        --feature-color: var(--tile-color);
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
        --feature-color: var(--tile-color);
      }
    `];ql([u({attribute:!1})],Fr.prototype,"flow",2);ql([g()],Fr.prototype,"_config",2);Fr=ql([x("hui-switch-card")],Fr);var mb=Object.defineProperty,bb=Object.getOwnPropertyDescriptor,Gl=(e,t,i,r)=>{for(var o=r>1?void 0:r?bb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&mb(t,i,o),o};let Br=class extends y{static getStubConfig(){return{type:"tile",entity:"/demo/light",name:"Demo Light",icon:"mdi:lightbulb"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _toggle(e){var i;e==null||e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,Cl(this._entityState()))}render(){var d,p;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=$e(t),r=String(this._config.name??((d=t==null?void 0:t.attributes)==null?void 0:d.friendly_name)??e??"Toggle"),o=String(this._config.icon??((p=t==null?void 0:t.attributes)==null?void 0:p.icon)??"mdi:help-circle"),s=!!this._config.hide_state?"":e&&this.flow?this.flow.formatState(e):"—",a=!!this._config.vertical,l=!!(this._config.show_icon_action??!0),c=P({"--tile-color":dr(i,this._config.color)});return h`
      <ha-card
        class=${T({active:i})}
        style=${c}
        tabindex="0"
        @click=${this._toggle}
        @keydown=${f=>{(f.key==="Enter"||f.key===" ")&&(f.preventDefault(),this._toggle())}}
      >
        <ha-tile-container ?vertical=${a} ?fixed-info-height=${a}>
          <ha-tile-icon
            slot="icon"
            .icon=${o}
            .active=${i}
            ?interactive=${l}
            @click=${l?this._toggle:void 0}
          ></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${r}
            .secondary=${s}
          ></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `}};Br.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
      }
      ha-card {
        height: 100%;
        min-height: 0;
      }
    `];Gl([u({attribute:!1})],Br.prototype,"flow",2);Gl([g()],Br.prototype,"_config",2);Br=Gl([x("hui-tile-card")],Br);function bs(e){return(e==null?void 0:e.trim())||Intl.DateTimeFormat().resolvedOptions().timeZone}function Yo(){return navigator.language||"zh-CN"}function vb(e){return e==="12"?!0:e==="24"?!1:new Intl.DateTimeFormat(Yo(),{hour:"numeric"}).resolvedOptions().hour12??!1}function dp(e){return e==="medium"||e==="large"}function hp(e,t="small",i,r=Yo()){if(!dp(t))return"";const o=t==="large"?{weekday:"long",day:"numeric",month:"long",timeZone:bs(i)}:{weekday:"short",day:"numeric",month:"short",timeZone:bs(i)};return new Intl.DateTimeFormat(r,o).format(e)}function _b(e,t,i=!1){var p;const o=new Intl.DateTimeFormat("en-US",{hour:"numeric",minute:"numeric",second:"numeric",hour12:i,timeZone:bs(t)}).formatToParts(e),n=f=>{var b;return Number(((b=o.find(v=>v.type===f))==null?void 0:b.value)??0)},s=n("hour"),a=n("minute"),l=n("second"),c=(p=o.find(f=>f.type==="dayPeriod"))==null?void 0:p.value,d=i?s:s%12||12;return{hour:s,minute:a,second:l,hour12:d,dayPeriod:c}}var yb=Object.defineProperty,xb=Object.getOwnPropertyDescriptor,Ct=(e,t,i,r)=>{for(var o=r>1?void 0:r?xb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&yb(t,i,o),o};const wb=1e3;let Re=class extends y{constructor(){super(...arguments),this.clockSize="small",this.showSeconds=!1,this._date=""}connectedCallback(){super.connectedCallback(),this._initFormatter(),this._startTick()}disconnectedCallback(){this._stopTick(),super.disconnectedCallback()}updated(e){(e.has("timeFormat")||e.has("timeZone")||e.has("showSeconds")||e.has("clockSize"))&&(this._initFormatter(),this._tick())}_initFormatter(){const e=vb(this.timeFormat);this._dateTimeFormat=new Intl.DateTimeFormat(Yo(),{hour:e?"numeric":"2-digit",minute:"2-digit",second:"2-digit",hourCycle:e?"h12":"h23",timeZone:bs(this.timeZone)}),this._lastDateMinute=void 0}_startTick(){this._stopTick(),this._tick(),this._tickInterval=window.setInterval(()=>this._tick(),wb)}_stopTick(){this._tickInterval&&(clearInterval(this._tickInterval),this._tickInterval=void 0)}_tick(){var i,r,o,n;if(!this._dateTimeFormat)return;const e=new Date,t=this._dateTimeFormat.formatToParts(e);this._timeHour=(i=t.find(s=>s.type==="hour"))==null?void 0:i.value,this._timeMinute=(r=t.find(s=>s.type==="minute"))==null?void 0:r.value,this._timeSecond=this.showSeconds?(o=t.find(s=>s.type==="second"))==null?void 0:o.value:void 0,this._timeAmPm=(n=t.find(s=>s.type==="dayPeriod"))==null?void 0:n.value,this._updateDate(e)}_updateDate(e){if(!dp(this.clockSize)){this._date="",this._lastDateMinute=void 0;return}this._timeMinute!==void 0&&this._timeMinute===this._lastDateMinute&&this._date||(this._date=hp(e,this.clockSize,this.timeZone,Yo()),this._lastDateMinute=this._timeMinute)}render(){const e=this.clockSize?`size-${this.clockSize}`:"",t=!!this._date;return h`
      <div class="clock-container">
        <div class=${T({"time-parts":!0,[e]:!!e})}>
          <span class="time-part hour">${this._timeHour??"--"}</span>
          <span class="time-part minute">${this._timeMinute??"--"}</span>
          ${this._timeSecond!==void 0?h`<span class="time-part second">${this._timeSecond}</span>`:m}
          ${this._timeAmPm!==void 0?h`<span class="time-part am-pm">${this._timeAmPm}</span>`:m}
        </div>
      </div>
      ${t?h`
            <div class="date-container">
              <div class=${T({date:!0,[e]:!!e})}>
                ${this._date}
              </div>
            </div>
          `:m}
    `}};Re.styles=w`
    :host {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      width: 100%;
      height: 100%;
      min-height: 0;
      box-sizing: border-box;
    }
    .clock-container {
      width: 100%;
      display: flex;
      justify-content: center;
    }
    .date-container {
      width: 100%;
      margin-top: var(--ha-space-1, 4px);
    }
    .time-parts {
      align-items: center;
      display: inline-grid;
      grid-template-areas:
        "hour minute second"
        "hour minute am-pm";
      font-size: 1.5rem;
      font-weight: var(--ha-font-weight-medium, 500);
      line-height: 0.8;
      direction: ltr;
      color: var(--primary-text-color);
      font-variant-numeric: tabular-nums;
    }
    .time-parts.size-medium {
      font-size: 3rem;
    }
    .time-parts.size-large {
      font-size: 4rem;
    }
    .time-parts.size-medium .time-part.second,
    .time-parts.size-medium .time-part.am-pm {
      font-size: var(--ha-font-size-l, 18px);
      margin-left: 6px;
    }
    .time-parts.size-large .time-part.second,
    .time-parts.size-large .time-part.am-pm {
      font-size: var(--ha-font-size-2xl, 28px);
      margin-left: 8px;
    }
    .time-part.hour {
      grid-area: hour;
    }
    .time-part.minute {
      grid-area: minute;
    }
    .time-part.second {
      grid-area: second;
      line-height: 0.9;
      opacity: 0.4;
      font-size: var(--ha-font-size-xs, 11px);
      margin-left: 4px;
    }
    .time-part.am-pm {
      grid-area: am-pm;
      line-height: 0.9;
      opacity: 0.6;
      font-size: var(--ha-font-size-xs, 11px);
      margin-left: 4px;
      text-transform: uppercase;
    }
    .time-part.hour::after {
      content: ":";
      margin: 0 2px;
    }
    .date {
      margin-inline: auto;
      text-align: center;
      opacity: 0.8;
      font-size: var(--ha-font-size-s, 12px);
      line-height: 1.1;
      overflow: hidden;
      white-space: nowrap;
      width: 100%;
      color: var(--primary-text-color);
    }
    .date.size-medium {
      font-size: var(--ha-font-size-l, 18px);
    }
    .date.size-large {
      font-size: var(--ha-font-size-2xl, 28px);
    }
  `;Ct([u({type:String})],Re.prototype,"clockSize",2);Ct([u({type:Boolean})],Re.prototype,"showSeconds",2);Ct([u({type:String})],Re.prototype,"timeFormat",2);Ct([u({type:String})],Re.prototype,"timeZone",2);Ct([g()],Re.prototype,"_timeHour",2);Ct([g()],Re.prototype,"_timeMinute",2);Ct([g()],Re.prototype,"_timeSecond",2);Ct([g()],Re.prototype,"_timeAmPm",2);Ct([g()],Re.prototype,"_date",2);Re=Ct([x("ha-clock-digital")],Re);var $b=Object.defineProperty,kb=Object.getOwnPropertyDescriptor,at=(e,t,i,r)=>{for(var o=r>1?void 0:r?kb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&$b(t,i,o),o};const Cb=6e4,Sb=[0,1,2,3],Eb=Array.from({length:12},(e,t)=>t),Pb=Array.from({length:60},(e,t)=>t);function Ab(e){const t=["","I","II","III","IV","V","VI","VII","VIII","IX","X","XI","XII"];return e<1||e>12?"":t[e]??""}let Me=class extends y{constructor(){super(...arguments),this.clockSize="small",this.showSeconds=!1,this.border=!0,this.ticks="hour",this.faceStyle="markers",this._hourOffsetSec=0,this._minuteOffsetSec=0,this._secondOffsetSec=0,this._date="",this._handleVisibilityChange=()=>{document.hidden||(this._computeOffsets(),this._updateDate())}}connectedCallback(){super.connectedCallback(),document.addEventListener("visibilitychange",this._handleVisibilityChange),this._computeOffsets(),this._updateDate(),this._startDateTick()}disconnectedCallback(){document.removeEventListener("visibilitychange",this._handleVisibilityChange),this._stopDateTick(),super.disconnectedCallback()}updated(e){(e.has("timeZone")||e.has("clockSize"))&&(this._computeOffsets(),this._updateDate())}_startDateTick(){this._stopDateTick(),this._dateTimer=window.setInterval(()=>this._updateDate(),Cb)}_stopDateTick(){this._dateTimer&&(clearInterval(this._dateTimer),this._dateTimer=void 0)}_computeOffsets(){const e=new Date,t=_b(e,this.timeZone,!0),i=e.getMilliseconds(),r=t.second+i/1e3,o=t.hour%12;this._secondOffsetSec=r,this._minuteOffsetSec=t.minute*60+r,this._hourOffsetSec=o*3600+t.minute*60+r}_updateDate(){this._date=hp(new Date,this.clockSize,this.timeZone,Yo())}_renderIndicator(e){if(!e||this.faceStyle==="markers")return m;const t=this.faceStyle==="roman"?Ab(e):String(e);return h`<div class="number">${t}</div>`}_renderTick(e,t){let i=0,r,o="tick";return t==="quarter"?(i=e*90,r=[12,3,6,9][e],o="tick hour"):t==="hour"?(i=e*30,r=(e+11)%12+1,o="tick hour"):t==="minute"&&(i=e*6,e%5===0?(r=(e/5+11)%12+1,o="tick hour"):o="tick minute"),h`
      <div class=${o} style=${`--tick-rotation: ${i}deg`}>
        <div
          class=${T({line:!0,numbers:this.faceStyle==="numbers",roman:this.faceStyle==="roman"})}
        ></div>
        ${this._renderIndicator(r)}
      </div>
    `}render(){const e=this.clockSize?`size-${this.clockSize}`:"",t=this.ticks??"hour",i=!!this._date,r=o=>P({animationDelay:`-${o}s`});return h`
      <div class=${T({"analog-clock":!0,[e]:!!e})}>
        <div class=${T({dial:!0,"dial-border":this.border})}>
          ${t==="quarter"?Sb.map(o=>this._renderTick(o,"quarter")):t==="minute"?Pb.map(o=>this._renderTick(o,"minute")):t==="none"?m:Eb.map(o=>this._renderTick(o,"hour"))}
          ${i?h`<div class=${T({date:!0,[e]:!!e})}>
                ${this._date}
              </div>`:m}
          <div class="center-dot"></div>
          <div class="hand hour" style=${r(this._hourOffsetSec)}></div>
          <div class="hand minute" style=${r(this._minuteOffsetSec)}></div>
          ${this.showSeconds?h`<div class="hand second" style=${r(this._secondOffsetSec)}></div>`:m}
        </div>
      </div>
    `}};Me.styles=w`
    :host {
      display: flex;
      align-items: center;
      justify-content: center;
      width: 100%;
      height: 100%;
      min-height: 0;
      box-sizing: border-box;
    }
    .analog-clock {
      --clock-size: 100px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: var(--clock-size);
      height: var(--clock-size);
    }
    .analog-clock.size-medium {
      --clock-size: 160px;
    }
    .analog-clock.size-large {
      --clock-size: 220px;
    }
    .dial {
      position: relative;
      width: 100%;
      height: 100%;
      box-sizing: border-box;
    }
    .dial-border {
      border: 2px solid var(--divider-color);
      border-radius: var(--ha-border-radius-circle, 50%);
    }
    .tick {
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      transform: rotate(var(--tick-rotation));
      pointer-events: none;
      z-index: 0;
    }
    .tick .line {
      position: absolute;
      top: 0;
      left: 50%;
      transform: translateX(-50%);
      width: 1px;
      height: calc(var(--clock-size) * 0.04);
      background: var(--primary-text-color);
      opacity: 0.5;
      border-radius: 1px;
    }
    .tick.hour .line {
      width: 2px;
      height: calc(var(--clock-size) * 0.07);
      opacity: 0.8;
    }
    .tick.hour .line.numbers,
    .tick.hour .line.roman {
      height: calc(var(--clock-size) * 0.03);
    }
    .tick.minute .line {
      height: calc(var(--clock-size) * 0.015);
      opacity: 0.35;
    }
    .tick .number {
      position: absolute;
      top: 0;
      left: 50%;
      transform: translate(-50%, 35%);
      color: var(--primary-text-color);
      font-weight: var(--ha-font-weight-medium, 500);
      font-size: var(--ha-font-size-s, 12px);
      line-height: var(--ha-line-height-condensed, 1.25);
    }
    .center-dot {
      position: absolute;
      top: 50%;
      left: 50%;
      width: 8px;
      height: 8px;
      border-radius: var(--ha-border-radius-circle, 50%);
      background: var(--primary-text-color);
      transform: translate(-50%, -50%);
      z-index: 3;
    }
    .hand {
      position: absolute;
      left: 50%;
      bottom: 50%;
      transform-origin: 50% 100%;
      transform: translate(-50%, 0) rotate(0deg);
      background: var(--primary-text-color);
      border-radius: 2px;
      will-change: transform;
      animation-name: ha-clock-rotate;
      animation-timing-function: linear;
      animation-iteration-count: infinite;
    }
    .hand.hour {
      width: 4px;
      height: calc(var(--clock-size) * 0.25);
      box-shadow: 0 0 8px rgba(0, 0, 0, 0.2);
      z-index: 1;
      animation-duration: 43200s;
    }
    .hand.minute {
      width: 3px;
      height: calc(var(--clock-size) * 0.35);
      box-shadow: 0 0 6px rgba(0, 0, 0, 0.2);
      opacity: 0.9;
      z-index: 3;
      animation-duration: 3600s;
    }
    .hand.second {
      width: 2px;
      height: calc(var(--clock-size) * 0.42);
      background: var(--ha-color-border-danger-normal, var(--error-color, #db4437));
      box-shadow: 0 0 4px rgba(0, 0, 0, 0.2);
      opacity: 0.8;
      z-index: 2;
      animation-duration: 60s;
    }
    @keyframes ha-clock-rotate {
      from {
        transform: translate(-50%, 0) rotate(0deg);
      }
      to {
        transform: translate(-50%, 0) rotate(360deg);
      }
    }
    .date {
      position: absolute;
      top: 68%;
      left: 50%;
      transform: translate(-50%, -50%);
      display: block;
      color: var(--primary-text-color);
      font-size: var(--ha-font-size-s, 12px);
      font-weight: var(--ha-font-weight-medium, 500);
      line-height: var(--ha-line-height-condensed, 1.25);
      text-align: center;
      opacity: 0.8;
      overflow: hidden;
      white-space: nowrap;
      width: 100%;
    }
    .date.size-medium {
      font-size: var(--ha-font-size-l, 18px);
    }
    .date.size-large {
      font-size: var(--ha-font-size-xl, 22px);
    }
  `;at([u({type:String})],Me.prototype,"clockSize",2);at([u({type:Boolean})],Me.prototype,"showSeconds",2);at([u({type:String})],Me.prototype,"timeZone",2);at([u({type:Boolean})],Me.prototype,"border",2);at([u({type:String})],Me.prototype,"ticks",2);at([u({type:String})],Me.prototype,"faceStyle",2);at([g()],Me.prototype,"_hourOffsetSec",2);at([g()],Me.prototype,"_minuteOffsetSec",2);at([g()],Me.prototype,"_secondOffsetSec",2);at([g()],Me.prototype,"_date",2);Me=at([x("ha-clock-analog")],Me);var Tb=Object.defineProperty,Mb=Object.getOwnPropertyDescriptor,Xl=(e,t,i,r)=>{for(var o=r>1?void 0:r?Mb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Tb(t,i,o),o};let Ur=class extends y{static getStubConfig(){return{type:"clock",clock_style:"digital",clock_size:"small",time_format:"24"}}setConfig(e){this._config={clock_style:"digital",clock_size:"small",...e}}getGridOptions(){var r,o,n,s;const e=String(((r=this._config)==null?void 0:r.clock_style)??"digital"),t=String(((o=this._config)==null?void 0:o.clock_size)??"small"),i=((n=this._config)==null?void 0:n.title)!==void 0&&((s=this._config)==null?void 0:s.title)!=="";if(e==="analog")switch(t){case"medium":return{columns:6,rows:i?4:3,min_columns:5,min_rows:i?4:3};case"large":return{columns:6,rows:i?5:4,min_columns:6,min_rows:i?5:4};default:return{columns:6,rows:i?3:2,min_columns:2,min_rows:i?3:2}}switch(t){case"medium":return{columns:6,rows:i?2:1,min_columns:4,min_rows:i?2:1,max_rows:4};case"large":return{columns:6,rows:2,min_columns:6,min_rows:2,max_rows:4};default:return{columns:6,rows:i?2:1,min_columns:3,min_rows:1,max_rows:4}}}render(){if(!this._config)return m;const e=String(this._config.clock_style??"digital"),t=String(this._config.clock_size??"small"),i=this._config.title!==void 0?String(this._config.title):void 0,r=!!this._config.no_background,o=!!this._config.show_seconds,n=this._config.time_format?String(this._config.time_format):void 0,s=this._config.time_zone?String(this._config.time_zone):void 0,a=this._config.analog_options??{},l=String(a.ticks??this._config.analog_ticks??"hour"),c=a.border!==void 0?!!a.border:this._config.analog_border!==void 0?!!this._config.analog_border:!0,d=String(a.face_style??this._config.analog_face_style??"markers");return h`
      <ha-card class=${T({"no-background":r})}>
        <div class=${T({"time-wrapper":!0,[`size-${t}`]:!0})}>
          ${i!==void 0?h`<div class="time-title">${i}</div>`:m}
          ${e==="analog"?h`
                <ha-clock-analog
                  .clockSize=${t}
                  .showSeconds=${o}
                  .timeZone=${s}
                  .border=${c}
                  .ticks=${l}
                  .faceStyle=${d}
                ></ha-clock-analog>
              `:h`
                <ha-clock-digital
                  .clockSize=${t}
                  .showSeconds=${o}
                  .timeFormat=${n}
                  .timeZone=${s}
                ></ha-clock-digital>
              `}
        </div>
      </ha-card>
    `}};Ur.styles=w`
    :host {
      display: block;
      height: 100%;
      min-height: 0;
    }
    ha-card {
      height: 100%;
      min-height: 0;
      display: flex;
      flex-direction: column;
      cursor: default;
      overflow: hidden;
    }
    ha-card.no-background {
      background: none;
      box-shadow: none;
      border: none;
    }
    .time-wrapper {
      flex: 1;
      min-height: 0;
      width: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-direction: column;
      padding: 6px 8px;
      row-gap: 6px;
      box-sizing: border-box;
    }
    .time-wrapper.size-medium,
    .time-wrapper.size-large {
      padding: 16px;
      row-gap: var(--ha-space-3, 12px);
    }
    .time-title {
      color: var(--primary-text-color);
      font-size: var(--ha-font-size-m, 14px);
      font-weight: var(--ha-font-weight-normal, 400);
      line-height: var(--ha-line-height-condensed, 1.25);
      overflow: hidden;
      text-align: center;
      text-overflow: ellipsis;
      white-space: nowrap;
      width: 100%;
      flex: none;
    }
    .time-wrapper.size-medium .time-title {
      font-size: var(--ha-font-size-l, 18px);
    }
    .time-wrapper.size-large .time-title {
      font-size: var(--ha-font-size-2xl, 28px);
    }
    ha-clock-analog,
    ha-clock-digital {
      flex: 1;
      min-height: 0;
      width: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
    }
  `;Xl([u({attribute:!1})],Ur.prototype,"flow",2);Xl([g()],Ur.prototype,"_config",2);Ur=Xl([x("hui-clock-card")],Ur);const Ob={off:"关闭",heat:"舒适",cool:"制冷",heat_cool:"自动",auto:"自动",dry:"除湿",fan_only:"送风"};function Wl(e){var i;const t=((i=e==null?void 0:e.attributes)==null?void 0:i.hvac_mode)??(e==null?void 0:e.state);return typeof t=="number"?t>0?"on":"off":String(t??"off").toLowerCase()}function Lb(e){const t=Wl(e);return t==="on"?"开启":Ob[t]??t}function Db(e){return typeof(e==null?void 0:e.state)=="number"?e.state>0:Wl(e)!=="off"}function Ib(e){var i,r;const t=((i=e==null?void 0:e.attributes)==null?void 0:i.current_temperature)??((r=e==null?void 0:e.attributes)==null?void 0:r.current_value);if(typeof t=="number")return t;if(t!=null)return Number(t)||void 0}function Hb(e,t=0){if(typeof(e==null?void 0:e.state)=="number")return e.state;const i=(e==null?void 0:e.attributes)??{},r=i.temperature??i.target_temp_high??i.target_temp_low??i.value;return typeof r=="number"?r:r!=null&&Number(r)||t}function zb(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.min_temp)??((o=e==null?void 0:e.attributes)==null?void 0:o.min);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:0}function Vb(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.max_temp)??((o=e==null?void 0:e.attributes)==null?void 0:o.max);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:100}function Nb(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.target_temp_step)??((o=e==null?void 0:e.attributes)==null?void 0:o.step);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:1}function Rb(e,t){var r;const i=(r=e==null?void 0:e.attributes)==null?void 0:r.unit_of_measurement;return typeof i=="string"&&i.trim()?i.trim():t!=null&&t.trim()?t.trim():""}function Fb(e,t=""){const i=Lb(e),r=Ib(e),o=t?` ${t}`:"",n=Wl(e);if(r!==void 0){const s=Number.isInteger(r)?0:1;return`${n!=="off"&&n!=="on"&&i!=="关闭"?`${i} · `:""}${r.toFixed(s)}${o}`.trim()}if(typeof(e==null?void 0:e.state)=="number"){const s=Number.isInteger(e.state)?0:1;return`${e.state.toFixed(s)}${o}`.trim()}return i}var Bb=Object.defineProperty,Ub=Object.getOwnPropertyDescriptor,_i=(e,t,i,r)=>{for(var o=r>1?void 0:r?Ub(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Bb(t,i,o),o};let ft=class extends y{constructor(){super(...arguments),this.value=0,this.min=0,this.max=100,this.step=1,this.disabled=!1,this.unit="",this.digits=0}_bounded(e){return Math.min(Math.max(e,this.min),this.max)}_stepped(e){return this._bounded(Math.round(e/this.step)*this.step)}_emit(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}_change(e,t){if(t.stopPropagation(),this.disabled)return;const i=this._stepped((this.value??this.min)+e);this.value=i,this._emit(i)}render(){const e=(this.value??this.min).toFixed(this.digits),t=this.unit?` ${this.unit}`:"";return h`
      <div class="container">
        <button
          type="button"
          class="btn"
          aria-label="减少"
          ?disabled=${this.disabled}
          @click=${i=>this._change(-this.step,i)}
        >
          −
        </button>
        <div class="value">${e}${t}</div>
        <button
          type="button"
          class="btn"
          aria-label="增加"
          ?disabled=${this.disabled}
          @click=${i=>this._change(this.step,i)}
        >
          +
        </button>
      </div>
    `}};ft.styles=w`
    :host {
      display: block;
      --control-color: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-background: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-background-opacity: 0.2;
      --control-height: var(--feature-height, 40px);
      --control-border-radius: var(
        --feature-border-radius,
        var(--ha-card-border-radius, var(--ha-border-radius-lg, 12px))
      );
      height: var(--control-height);
      width: 100%;
    }
    .container {
      display: grid;
      grid-template-columns: 1fr auto 1fr;
      align-items: center;
      height: 100%;
      border-radius: var(--control-border-radius);
      background: color-mix(in srgb, var(--control-background) calc(var(--control-background-opacity) * 100%), transparent);
      overflow: hidden;
    }
    .btn {
      appearance: none;
      border: none;
      background: transparent;
      color: var(--control-color);
      font-size: 22px;
      line-height: 1;
      height: 100%;
      cursor: pointer;
      padding: 0 12px;
    }
    .btn:hover:not(:disabled) {
      background: color-mix(in srgb, var(--control-color) 12%, transparent);
    }
    .btn:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
    .value {
      color: var(--primary-text-color);
      font-size: 16px;
      font-weight: 500;
      white-space: nowrap;
      text-align: center;
      padding: 0 8px;
      user-select: none;
    }
  `;_i([u({type:Number})],ft.prototype,"value",2);_i([u({type:Number})],ft.prototype,"min",2);_i([u({type:Number})],ft.prototype,"max",2);_i([u({type:Number})],ft.prototype,"step",2);_i([u({type:Boolean})],ft.prototype,"disabled",2);_i([u({type:String})],ft.prototype,"unit",2);_i([u({type:Number})],ft.prototype,"digits",2);ft=_i([x("ha-control-number-buttons")],ft);var jb=Object.defineProperty,qb=Object.getOwnPropertyDescriptor,Yl=(e,t,i,r)=>{for(var o=r>1?void 0:r?qb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&jb(t,i,o),o};function pa(e,t){if(e==null||e==="")return t;const i=Number(e);return Number.isNaN(i)?t:i}let jr=class extends y{static getStubConfig(){return{type:"climate",entity:"/demo/climate",name:"Upstairs",icon:"mdi:home-thermometer",min:0,max:100,step:1,unit:"°C"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _onTargetChange(e){var i;e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,e.detail.value)}render(){var _,k;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=Db(t),r=!!this._config.vertical,o=String(this._config.name??((_=t==null?void 0:t.attributes)==null?void 0:_.friendly_name)??e??"Climate"),n=String(this._config.icon??((k=t==null?void 0:t.attributes)==null?void 0:k.icon)??"mdi:home-thermometer"),s=zb(t,pa(this._config.min,0)),a=Vb(t,pa(this._config.max,100)),l=Nb(t,pa(this._config.step,1)),c=Rb(t,String(this._config.unit??"")),d=Hb(t,s),p=l>=1?0:1,f=hr(i,this._config.color,"orange"),b=P({"--tile-color":f}),v=P({"--feature-color":f??(i?"var(--orange-color, #ff9800)":"var(--state-inactive-color)")});return h`
      <ha-card class=${T({active:i})} style=${b}>
        <ha-tile-container ?vertical=${r} ?fixed-info-height=${r}>
          <ha-tile-icon slot="icon" .icon=${n} .active=${i}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${o}
            .secondary=${Fb(t,c)}
          ></ha-tile-info>
          <div slot="features" class="feature" style=${v}>
            <ha-control-number-buttons
              .value=${d}
              .min=${s}
              .max=${a}
              .step=${l}
              .unit=${c}
              .digits=${p}
              @value-changed=${this._onTargetChange}
              @click=${C=>C.stopPropagation()}
            ></ha-control-number-buttons>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};jr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
      }
    `];Yl([u({attribute:!1})],jr.prototype,"flow",2);Yl([g()],jr.prototype,"_config",2);jr=Yl([x("hui-climate-card")],jr);const Zo=4;function pp(e=Zo){const t=Math.max(1,Math.min(12,Math.round(e)||Zo));return Array.from({length:t},(i,r)=>r)}const Ko=pp(Zo);function Gb(e){const t=e==null?void 0:e.positions;if(Array.isArray(t)&&t.length>0){const i=t.map(r=>Number(r)).filter(r=>!Number.isNaN(r));if(i.length>0)return i}return pp(Number((e==null?void 0:e.position_count)??Zo))}function Zl(e,t=Ko){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.current_position)??((o=e==null?void 0:e.attributes)==null?void 0:o.position);return typeof i=="number"?Math.round(i):i!=null?Math.round(Number(i)||0):typeof(e==null?void 0:e.state)=="number"?Math.round(e.state):$e(e)?t[t.length-1]??0:t[0]??0}function Xb(e,t=Ko){const i=Zl(e,t),r=t[0]??0;return i>r}function Wb(e,t=Ko){const i=Zl(e,t),r=t[0]??0;return`${i>r?"已打开":"已关闭"} · ${i}`}var Yb=Object.defineProperty,Zb=Object.getOwnPropertyDescriptor,js=(e,t,i,r)=>{for(var o=r>1?void 0:r?Zb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Yb(t,i,o),o};let qr=class extends y{constructor(){super(...arguments),this.value=0,this.positions=Ko,this.disabled=!1}_emit(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}_select(e,t){t.stopPropagation(),!this.disabled&&(this.value=e,this._emit(e))}render(){var t;const e=(t=this.positions)!=null&&t.length?this.positions:Ko;return h`
      <div class="container" role="group" aria-label="单选位置">
        ${e.map(i=>h`
            <button
              type="button"
              class=${T({segment:!0,selected:this.value===i})}
              ?disabled=${this.disabled}
              @click=${r=>this._select(i,r)}
            >
              ${i}
            </button>
          `)}
      </div>
    `}};qr.styles=w`
    :host {
      display: block;
      --control-color: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-background: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-background-opacity: 0.2;
      --control-height: var(--feature-height, 40px);
      --control-border-radius: var(
        --feature-border-radius,
        var(--ha-card-border-radius, var(--ha-border-radius-lg, 12px))
      );
      height: var(--control-height);
      width: 100%;
    }
    .container {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(0, 1fr));
      gap: 4px;
      height: 100%;
      padding: 4px;
      box-sizing: border-box;
      border-radius: var(--control-border-radius);
      background: color-mix(in srgb, var(--control-background) calc(var(--control-background-opacity) * 100%), transparent);
    }
    .segment {
      appearance: none;
      border: none;
      border-radius: calc(var(--control-border-radius) - 4px);
      background: transparent;
      color: var(--secondary-text-color);
      font-size: 14px;
      font-weight: 500;
      cursor: pointer;
      min-width: 0;
      padding: 0 4px;
      transition:
        background-color 180ms ease,
        color 180ms ease;
    }
    .segment.selected {
      background: var(--control-color);
      color: white;
    }
    .segment:hover:not(:disabled):not(.selected) {
      background: color-mix(in srgb, var(--control-color) 12%, transparent);
    }
    .segment:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
  `;js([u({type:Number})],qr.prototype,"value",2);js([u({type:Array})],qr.prototype,"positions",2);js([u({type:Boolean})],qr.prototype,"disabled",2);qr=js([x("ha-control-cover-position")],qr);var Kb=Object.defineProperty,Qb=Object.getOwnPropertyDescriptor,Kl=(e,t,i,r)=>{for(var o=r>1?void 0:r?Qb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Kb(t,i,o),o};let Gr=class extends y{static getStubConfig(){return{type:"cover",entity:"/demo/kitchen_shutter",name:"Kitchen shutter",icon:"mdi:window-shutter",position_count:Zo}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}_positions(){return Gb(this._config)}async _onPositionChange(e){var i;e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,e.detail.value)}render(){var p,f;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=this._positions(),r=Xb(t,i),o=!!this._config.vertical,n=String(this._config.name??((p=t==null?void 0:t.attributes)==null?void 0:p.friendly_name)??e??"Cover"),s=String(this._config.icon??((f=t==null?void 0:t.attributes)==null?void 0:f.icon)??"mdi:window-shutter"),a=Zl(t,i),l=hr(r,this._config.color,"deep-purple"),c=P({"--tile-color":l}),d=P({"--feature-color":l??(r?"var(--deep-purple-color, #7e57c2)":"var(--state-inactive-color)")});return h`
      <ha-card class=${T({active:r})} style=${c}>
        <ha-tile-container ?vertical=${o} ?fixed-info-height=${o}>
          <ha-tile-icon slot="icon" .icon=${s} .active=${r}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${n}
            .secondary=${Wb(t,i)}
          ></ha-tile-info>
          <div slot="features" class="feature" style=${d}>
            <ha-control-cover-position
              .value=${a}
              .positions=${i}
              @value-changed=${this._onPositionChange}
              @click=${b=>b.stopPropagation()}
            ></ha-control-cover-position>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Gr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
      }
    `];Kl([u({attribute:!1})],Gr.prototype,"flow",2);Kl([g()],Gr.prototype,"_config",2);Gr=Kl([x("hui-cover-card")],Gr);var Jb=Object.defineProperty,e1=Object.getOwnPropertyDescriptor,qs=(e,t,i,r)=>{for(var o=r>1?void 0:r?e1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Jb(t,i,o),o};const t1=700;let Zi=class extends y{constructor(){super(...arguments),this._flashing=!1,this._lastActive=!1,this._entityReady=!1}static getStubConfig(){return{type:"trigger",entity:"/demo/trigger",name:"Trigger",icon:"mdi:gesture-tap-button"}}setConfig(e){this._config=e,this.requestUpdate(),this._syncEntitySubscription()}connectedCallback(){super.connectedCallback(),this._syncEntitySubscription()}disconnectedCallback(){this._clearEntitySubscription(),this._pulseTimer&&clearTimeout(this._pulseTimer),super.disconnectedCallback()}updated(e){e.has("flow")&&this._syncEntitySubscription()}_clearEntitySubscription(){var e;(e=this._entityUnsub)==null||e.call(this),this._entityUnsub=void 0,this._entityReady=!1}_syncEntitySubscription(){var t;this._clearEntitySubscription();const e=(t=this._config)==null?void 0:t.entity;!e||!this.flow||(this._entityUnsub=this.flow.subscribeEntity(e,i=>{const r=$e(i);this._entityReady?r&&!this._lastActive&&this._pulse():this._entityReady=!0,this._lastActive=r}))}_pulse(){this._flashing=!0,this._pulseTimer&&clearTimeout(this._pulseTimer),this._pulseTimer=setTimeout(()=>{this._flashing=!1,this._pulseTimer=void 0,this.requestUpdate()},t1),this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _trigger(e){var i;e==null||e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||(await this.flow.callService(t,!0),this._pulse())}render(){var c,d;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=$e(t),r=this._flashing||i,o=String(this._config.name??((c=t==null?void 0:t.attributes)==null?void 0:c.friendly_name)??e??"Trigger"),n=String(this._config.icon??((d=t==null?void 0:t.attributes)==null?void 0:d.icon)??"mdi:gesture-tap-button"),s=!!this._config.vertical,a=r?"已触发":"未触发",l=P({"--tile-color":dr(r,this._config.color)});return h`
      <ha-card
        class=${T({active:r,pulse:this._flashing})}
        style=${l}
        tabindex="0"
        @click=${this._trigger}
        @keydown=${p=>{(p.key==="Enter"||p.key===" ")&&(p.preventDefault(),this._trigger())}}
      >
        <ha-tile-container ?vertical=${s} ?fixed-info-height=${s}>
          <ha-tile-icon
            slot="icon"
            .icon=${n}
            .active=${r}
            interactive
          ></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${o}
            .secondary=${a}
          ></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `}};Zi.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
      }
      ha-card {
        height: 100%;
        min-height: 0;
        cursor: pointer;
        transition:
          box-shadow 200ms ease-in-out,
          border-color 200ms ease-in-out;
      }
      ha-card.pulse {
        transition:
          box-shadow 120ms ease-in-out,
          border-color 120ms ease-in-out;
      }
      ha-card:hover {
        box-shadow:
          var(--ha-card-box-shadow),
          0 2px 6px rgba(0, 0, 0, 0.06);
      }
    `];qs([u({attribute:!1})],Zi.prototype,"flow",2);qs([g()],Zi.prototype,"_config",2);qs([g()],Zi.prototype,"_flashing",2);Zi=qs([x("hui-trigger-card")],Zi);function Y(e,t,i){e.dispatchEvent(new CustomEvent(t,{detail:i,bubbles:!0,composed:!0}))}const Qo=`data:image/svg+xml,${encodeURIComponent(`
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 560">
  <rect width="900" height="560" fill="#ead9c2"/>
  <g fill="#f4ece1" stroke="#4c3828" stroke-width="7">
    <rect x="24" y="24" width="368" height="268"/>
    <rect x="392" y="24" width="484" height="268"/>
    <rect x="24" y="292" width="248" height="244"/>
    <rect x="272" y="292" width="278" height="244"/>
    <rect x="550" y="292" width="326" height="244"/>
  </g>
  <g fill="none" stroke="#4c3828" stroke-width="7">
    <rect x="24" y="24" width="852" height="512" rx="4"/>
  </g>
  <g fill="#6d5644" font-family="Segoe UI, sans-serif" font-size="22" text-anchor="middle">
    <text x="208" y="164">客厅</text>
    <text x="634" y="164">厨房</text>
    <text x="148" y="424">卧室</text>
    <text x="411" y="424">卫生间</text>
    <text x="713" y="424">阳台</text>
  </g>
</svg>
`)}`;function dt(e,t=50){if(typeof e=="number"&&Number.isFinite(e))return e;if(typeof e=="string"){const i=parseFloat(e);if(Number.isFinite(i))return i}return t}function vs(e){return Math.min(100,Math.max(0,e))}function i1(e){var t,i;return{top:dt((t=e.style)==null?void 0:t.top,50),left:dt((i=e.style)==null?void 0:i.left,50)}}function ts(e,t,i){return{...e,style:{...e.style,top:`${vs(t).toFixed(1)}%`,left:`${vs(i).toFixed(1)}%`}}}function up(e,t,i){if(!t||!e)return{value:"—",unit:"",numeric:!1};const r=e.states[t];if(!r)return{value:"—",unit:"",numeric:!1};const o=String(i??r.attributes.unit_of_measurement??"").trim(),n=r.state;if(typeof n=="boolean")return{value:n?"开":"关",unit:"",numeric:!1};if(n==null)return{value:"—",unit:"",numeric:!1};if(typeof n=="number")return{value:Number.isInteger(n)?String(n):n.toFixed(1).replace(/\.0$/,""),unit:o,numeric:!0};const s=Number(n);return n!==""&&Number.isFinite(s)?{value:Number.isInteger(s)?String(s):s.toFixed(1).replace(/\.0$/,""),unit:o,numeric:!0}:{value:String(n),unit:o,numeric:!1}}var r1=Object.defineProperty,o1=Object.getOwnPropertyDescriptor,St=(e,t,i,r)=>{for(var o=r>1?void 0:r?o1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&r1(t,i,o),o};let Fe=class extends y{constructor(){super(...arguments),this.value="—",this.unit="",this.label="",this.icon="",this.color="",this.showValue=!0,this.showIcon=!1,this.size=qi,this.showBackground=!0}render(){const e=this.color?Nt(this.color):"var(--state-icon-color, var(--primary-color))",t=Math.min(80,Math.max(18,this.size||qi)),i=Math.max(12,Math.round(t*16/26)),r=Math.max(11,Math.round(t*12/26));return h`
      <div
        class=${T({badge:!0,transparent:!this.showBackground,"with-text":this.showValue||!!this.unit})}
        style=${P({"--badge-color":e,"--badge-height":`${t}px`,"--badge-icon-size":`${i}px`,"--badge-font-size":`${r}px`})}
      >
        ${this.showIcon&&this.icon?h`<ha-icon .icon=${this.icon}></ha-icon>`:m}
        ${this.showValue?h`<span class="value">${this.value}</span>`:m}
        ${this.showValue&&this.unit?h`<span class="unit">${this.unit}</span>`:m}
      </div>
      ${this.label?h`<div class="label">${this.label}</div>`:m}
    `}};Fe.styles=w`
    :host {
      display: inline-flex;
      flex-direction: column;
      align-items: center;
      gap: 2px;
    }
    .badge {
      display: inline-flex;
      flex-direction: row;
      align-items: center;
      justify-content: center;
      gap: 2px;
      height: var(--badge-height, 26px);
      min-width: var(--badge-height, 26px);
      padding: 0;
      border: none;
      border-radius: 999px;
      box-sizing: border-box;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--badge-color, var(--secondary-text-color));
      font-size: var(--badge-font-size, 12px);
      font-weight: 500;
      line-height: 1;
      white-space: nowrap;
      user-select: none;
    }
    .badge.with-text {
      padding: 0 8px;
    }
    .badge.transparent {
      background: transparent;
    }
    .value {
      padding: 0 2px;
      max-width: 96px;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .unit {
      padding: 0 2px 0 0;
      opacity: 0.85;
      font-weight: 600;
    }
    ha-icon {
      --mdc-icon-size: var(--badge-icon-size, 16px);
      color: var(--badge-color, inherit);
    }
    .label {
      font-size: 11px;
      font-weight: 500;
      color: var(--primary-text-color);
      max-width: 100px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      text-align: center;
    }
  `;St([u()],Fe.prototype,"value",2);St([u()],Fe.prototype,"unit",2);St([u()],Fe.prototype,"label",2);St([u()],Fe.prototype,"icon",2);St([u()],Fe.prototype,"color",2);St([u({type:Boolean})],Fe.prototype,"showValue",2);St([u({type:Boolean})],Fe.prototype,"showIcon",2);St([u({type:Number})],Fe.prototype,"size",2);St([u({type:Boolean})],Fe.prototype,"showBackground",2);Fe=St([x("ha-state-badge")],Fe);var n1=Object.defineProperty,s1=Object.getOwnPropertyDescriptor,yn=(e,t,i,r)=>{for(var o=r>1?void 0:r?s1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&n1(t,i,o),o};let Ki=class extends y{constructor(){super(...arguments),this.statesRevision=0,this._tick=0}connectedCallback(){super.connectedCallback(),this._syncSubscription()}disconnectedCallback(){var e;(e=this._unsub)==null||e.call(this),this._unsub=void 0,this._subscribedEntity=void 0,super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("config"))&&this._syncSubscription()}_entityId(){var e;return(e=this.config)!=null&&e.entity?String(this.config.entity).trim():""}_badgeColor(){var o;const e=(o=this.config)!=null&&o.color?String(this.config.color).trim():"",t=this._entityId(),i=t&&this.flow?this.flow.states[t]:void 0;return $e(i)?!e||e==="none"?"":e==="state"?"var(--state-icon-color)":e:"var(--state-inactive-color)"}_syncSubscription(){var t;const e=this._entityId();e===this._subscribedEntity&&this._unsub||((t=this._unsub)==null||t.call(this),this._unsub=void 0,this._subscribedEntity=void 0,!(!e||!this.flow)&&(this._subscribedEntity=e,this._unsub=this.flow.subscribeEntity(e,()=>{this._tick+=1})))}render(){var l;const e=this.config;if(!e)return m;this._tick,this.statesRevision;const t=this._entityId(),i=up(this.flow,t,e.unit),r=t&&this.flow?this.flow.states[t]:void 0,o=String(e.icon??((l=r==null?void 0:r.attributes)==null?void 0:l.icon)??"mdi:eye"),n=this._badgeColor(),s=Sl(e),a=Rt(e);return h`
      <ha-state-badge
        .value=${i.value}
        .unit=${i.unit}
        .icon=${o}
        .color=${n}
        .showIcon=${s}
        .showValue=${a}
        .size=${fn(e)}
        .showBackground=${di(e)}
      ></ha-state-badge>
    `}};Ki.styles=w`
    :host {
      display: inline-flex;
    }
  `;yn([u({attribute:!1})],Ki.prototype,"flow",2);yn([u({attribute:!1})],Ki.prototype,"config",2);yn([u({type:Number,attribute:!1})],Ki.prototype,"statesRevision",2);yn([g()],Ki.prototype,"_tick",2);Ki=yn([x("hui-picture-state-element")],Ki);var a1=Object.defineProperty,l1=Object.getOwnPropertyDescriptor,yi=(e,t,i,r)=>{for(var o=r>1?void 0:r?l1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&a1(t,i,o),o};let gt=class extends y{constructor(){super(...arguments),this.preview=!1,this.statesRevision=0,this._flashing=!1,this._tick=0}connectedCallback(){super.connectedCallback(),this._syncSubscription()}disconnectedCallback(){var e;(e=this._unsub)==null||e.call(this),this._unsub=void 0,this._subscribedEntity=void 0,this._pulseTimer&&clearTimeout(this._pulseTimer),super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("config"))&&this._syncSubscription(),this._syncLatchedFromState()}_entityId(){var e;return(e=this.config)!=null&&e.entity?String(this.config.entity).trim():""}_entityState(){const e=this._entityId();if(!(!e||!this.flow))return this.flow.states[e]}_syncSubscription(){var t;const e=this._entityId();e===this._subscribedEntity&&this._unsub||((t=this._unsub)==null||t.call(this),this._unsub=void 0,this._subscribedEntity=void 0,!(!e||!this.flow)&&(this._subscribedEntity=e,this._unsub=this.flow.subscribeEntity(e,()=>{this._syncLatchedFromState(),this._tick+=1})))}_syncLatchedFromState(){if(typeof this._latched!="boolean")return;Pl(this.config,this._entityState())===this._latched&&(this._latched=void 0)}_iconColor(e){var i;return e||$e(this._entityState())?dr(!0,(i=this.config)==null?void 0:i.color)??"var(--state-icon-color, var(--primary-color))":"var(--state-inactive-color, var(--secondary-text-color))"}_pulse(){this._flashing=!0,this._pulseTimer&&clearTimeout(this._pulseTimer),this._pulseTimer=setTimeout(()=>{this._flashing=!1,this._pulseTimer=void 0},700)}async _tap(e){e.stopPropagation();const t=await Hh(this.flow,this.config,{preview:this.preview,latched:this._latched});if(t.mode==="pulse"){this._pulse();return}t.mode==="latch"&&(this._latched=t.pressed)}render(){const e=this.config;if(!e)return m;this._tick,this.statesRevision;const t=Ge(e),i=t.icon||"mdi:gesture-tap-button",r=t.text||t.name||"",o=Rt(e)&&!!r,n=di(e),s=fn(e,qi),a=Math.max(12,Math.round(s*16/26)),l=Math.max(11,Math.round(s*12/26)),c=xl(t.tap_action.action),d=zh(e,this._entityState(),{flashing:this._flashing,latched:this._latched}),p=this._iconColor(d),f=Vh(e,this.preview);return h`
      <button
        type="button"
        class=${T({badge:!0,"with-text":o,transparent:!n,flashing:!c&&this._flashing,pressed:c&&d})}
        style=${P({"--color":p,"--badge-height":`${s}px`,"--badge-icon-size":`${a}px`,"--badge-font-size":`${l}px`})}
        ?disabled=${!f}
        aria-pressed=${c?d?"true":"false":"undefined"}
        @click=${this._tap}
      >
        <ha-icon .icon=${i}></ha-icon>
        ${o?h`<span class="text">${r}</span>`:m}
      </button>
    `}};gt.styles=w`
    :host {
      display: inline-flex;
    }
    .badge {
      display: inline-flex;
      flex-direction: row;
      align-items: center;
      justify-content: center;
      gap: 2px;
      height: var(--badge-height, 26px);
      min-width: var(--badge-height, 26px);
      padding: 0;
      border: none;
      border-radius: 999px;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--color, var(--secondary-text-color));
      cursor: pointer;
      font: inherit;
      font-size: var(--badge-font-size, 12px);
      font-weight: 500;
      line-height: 1;
      white-space: nowrap;
      user-select: none;
    }
    .badge.with-text {
      padding: 0 8px;
    }
    .badge.transparent {
      background: transparent;
    }
    .badge:not(.transparent).flashing,
    .badge:not(.transparent).pressed {
      background: color-mix(in srgb, var(--color, var(--primary-color)) 22%, transparent);
    }
    .badge:disabled {
      cursor: grab;
      opacity: 0.55;
    }
    .badge:not(:disabled):hover {
      filter: brightness(0.96);
    }
    .badge.transparent:not(:disabled):hover {
      filter: none;
      opacity: 0.85;
    }
    ha-icon {
      --mdc-icon-size: var(--badge-icon-size, 16px);
    }
    .text {
      padding: 0 4px;
      max-width: 96px;
      overflow: hidden;
      text-overflow: ellipsis;
    }
  `;yi([u({attribute:!1})],gt.prototype,"flow",2);yi([u({attribute:!1})],gt.prototype,"config",2);yi([u({type:Boolean})],gt.prototype,"preview",2);yi([u({type:Number,attribute:!1})],gt.prototype,"statesRevision",2);yi([g()],gt.prototype,"_flashing",2);yi([g()],gt.prototype,"_latched",2);yi([g()],gt.prototype,"_tick",2);gt=yi([x("hui-picture-action-element")],gt);var c1=Object.defineProperty,d1=Object.getOwnPropertyDescriptor,gr=(e,t,i,r)=>{for(var o=r>1?void 0:r?d1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&c1(t,i,o),o};let mt=class extends y{constructor(){super(...arguments),this.preview=!1,this._elements=[],this._highlightIndex=-1,this._dragIndex=null,this._moved=!1,this._onHighlight=e=>{var i;if(!this.preview)return;const t=Number((i=e.detail)==null?void 0:i.index);this._highlightIndex=Number.isFinite(t)?t:-1}}static getStubConfig(){return{type:"picture-elements",image:Qo,elements:[Kn({entity:"/demo/temperature",name:"温度",icon:"mdi:thermometer",color:"red",style:{top:"32%",left:"24%"}}),Va({entity:"/demo/trigger",name:"触发",text:"触发",style:{top:"58%",left:"72%"}})],grid_options:{columns:12,rows:"auto",min_columns:6,min_rows:3}}}setConfig(e){this._config=e,this._dragIndex===null&&(this._elements=(e.elements??[]).map(t=>bi(Ge(t))))}connectedCallback(){super.connectedCallback(),document.addEventListener("flow-picture-badge-highlight",this._onHighlight)}disconnectedCallback(){document.removeEventListener("flow-picture-badge-highlight",this._onHighlight),super.disconnectedCallback()}getGridOptions(){return{columns:12,rows:"auto",min_columns:6,min_rows:3}}_emitConfig(){this._config&&Y(this,"picture-elements-changed",{config:{...this._config,elements:this._elements.map(e=>({...e}))}})}_pointPercent(e){const t=this._root;if(!t)return;const i=t.getBoundingClientRect();if(!(i.width<=0||i.height<=0))return{left:vs((e.clientX-i.left)/i.width*100),top:vs((e.clientY-i.top)/i.height*100)}}_onElementPointerDown(e,t){!this.preview||t.button!==0||(t.preventDefault(),t.stopPropagation(),this._dragIndex=e,this._moved=!1,this._highlightIndex=e,Y(this,"picture-element-select",{index:e}),document.dispatchEvent(new CustomEvent("flow-picture-badge-highlight",{detail:{index:e}})),t.currentTarget.setPointerCapture(t.pointerId))}_onElementPointerMove(e){if(this._dragIndex===null)return;const t=this._pointPercent(e);t&&(this._moved=!0,this._elements=this._elements.map((i,r)=>r===this._dragIndex?ts(i,t.top,t.left):i))}_onElementPointerUp(e){this._dragIndex!==null&&(e.stopPropagation(),this._dragIndex=null,this._moved&&this._emitConfig())}_onRootPointerUp(e){if(!this.preview||e.button!==0||this._dragIndex!==null||this._moved)return;const t=e.target;if(t!=null&&t.closest(".element"))return;const i=this._pointPercent(e);i&&Y(this,"picture-element-position",i)}_renderElement(e,t){var o,n;const i=i1(e),r=oi(e)?h`
            <hui-picture-action-element
              .flow=${this.flow}
              .config=${e}
              .preview=${this.preview}
              .statesRevision=${((o=this.flow)==null?void 0:o.statesRevision)??0}
            ></hui-picture-action-element>
          `:h`
            <hui-picture-state-element
              .flow=${this.flow}
              .config=${e}
              .statesRevision=${((n=this.flow)==null?void 0:n.statesRevision)??0}
            ></hui-picture-state-element>
          `;return h`
      <div
        class=${T({element:!0,preview:this.preview,selected:this.preview&&this._highlightIndex===t})}
        style=${P({top:`${i.top}%`,left:`${i.left}%`})}
        @pointerdown=${s=>this._onElementPointerDown(t,s)}
        @pointermove=${this._onElementPointerMove}
        @pointerup=${this._onElementPointerUp}
        @pointercancel=${this._onElementPointerUp}
      >
        ${r}
      </div>
    `}render(){var i;if(!this._config)return m;(i=this.flow)==null||i.statesRevision;const e=this._config.title?String(this._config.title):"",t=this._config.image?String(this._config.image):"";return h`
      <ha-card>
        ${e?h`<h1 class="card-header">${e}</h1>`:m}
        <div class="root" @pointerup=${this._onRootPointerUp}>
          ${t?h`<img alt="" src=${t} draggable="false" />`:h`<div class="placeholder">添加底图后可在图上放置徽章</div>`}
          ${this._elements.map((r,o)=>this._renderElement(r,o))}
        </div>
      </ha-card>
    `}};mt.styles=w`
    :host {
      display: block;
      height: 100%;
    }
    ha-card {
      height: 100%;
      overflow: hidden;
    }
    .card-header {
      margin: 0;
      padding: 12px 16px 0;
      font-size: 16px;
      font-weight: 600;
    }
    .root {
      position: relative;
      overflow: hidden;
    }
    img {
      display: block;
      width: 100%;
      height: auto;
      user-select: none;
      pointer-events: none;
    }
    .placeholder {
      min-height: 180px;
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--secondary-text-color);
      font-size: 13px;
      background: repeating-linear-gradient(
        45deg,
        var(--secondary-background-color, #f3f4f6),
        var(--secondary-background-color, #f3f4f6) 12px,
        transparent 12px,
        transparent 24px
      );
    }
    .element {
      position: absolute;
      transform: translate(-50%, -50%);
      z-index: 1;
      touch-action: none;
    }
    .element.preview {
      cursor: grab;
    }
    .element.preview:active {
      cursor: grabbing;
    }
    .element.selected {
      z-index: 3;
      outline: 2px solid var(--primary-color);
      outline-offset: 4px;
      border-radius: 999px;
      box-shadow: 0 0 0 4px color-mix(in srgb, var(--primary-color) 25%, transparent);
    }
  `;gr([u({attribute:!1})],mt.prototype,"flow",2);gr([u({type:Boolean})],mt.prototype,"preview",2);gr([g()],mt.prototype,"_config",2);gr([g()],mt.prototype,"_elements",2);gr([g()],mt.prototype,"_highlightIndex",2);gr([mi(".root")],mt.prototype,"_root",2);mt=gr([x("hui-picture-elements-card")],mt);var h1=Object.defineProperty,p1=Object.getOwnPropertyDescriptor,Ql=(e,t,i,r)=>{for(var o=r>1?void 0:r?p1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&h1(t,i,o),o};function rd(e){const t=e.trim();if(!t)return"未设置链接";try{const i=new URL(t.includes("://")?t:`https://${t}`);return i.hostname+(i.pathname!=="/"?i.pathname:"")}catch{return t.length>48?`${t.slice(0,45)}…`:t}}function u1(e){const t=e.trim();return t?/^[a-z][a-z0-9+.-]*:/i.test(t)?t:`https://${t}`:""}let Xr=class extends y{static getStubConfig(){return{type:"link",name:"超链接",url:"http://127.0.0.1:8992/",icon:"mdi:link-variant",new_tab:!0}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){return{columns:6,rows:1,min_columns:3,min_rows:1}}render(){if(!this._config)return m;const e=String(this._config.name??"超链接"),t=String(this._config.url??""),i=u1(t),r=String(this._config.icon??"mdi:link-variant"),o=this._config.new_tab!==!1,n=!i;return h`
      <ha-card class=${n?"disabled":""}>
        ${n?h`
              <div class="link-body static">
                <ha-tile-container>
                  <ha-tile-icon slot="icon" .icon=${r}></ha-tile-icon>
                  <ha-tile-info slot="info" .primary=${e} .secondary=${rd(t)}></ha-tile-info>
                </ha-tile-container>
              </div>
            `:h`
              <a
                class="link-body"
                href=${i}
                target=${o?"_blank":"_self"}
                rel=${o?"noopener noreferrer":m}
                @click=${s=>s.stopPropagation()}
              >
                <ha-tile-container>
                  <ha-tile-icon slot="icon" .icon=${r} .active=${!0}></ha-tile-icon>
                  <ha-tile-info slot="info" .primary=${e} .secondary=${rd(t)}></ha-tile-info>
                </ha-tile-container>
              </a>
            `}
      </ha-card>
    `}};Xr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
      }
      ha-card {
        height: 100%;
        min-height: 0;
        padding: 0;
        overflow: hidden;
      }
      ha-card.disabled {
        opacity: 0.72;
      }
      .link-body {
        display: block;
        height: 100%;
        min-height: 0;
        color: inherit;
        text-decoration: none;
        box-sizing: border-box;
      }
      .link-body.static {
        cursor: default;
      }
      a.link-body {
        cursor: pointer;
      }
      a.link-body:hover ha-card,
      a.link-body:focus-visible {
        outline: none;
      }
      a.link-body:hover {
        filter: brightness(0.98);
      }
      ha-tile-container {
        height: 100%;
        min-height: 0;
      }
    `];Ql([u({attribute:!1})],Xr.prototype,"flow",2);Ql([g()],Xr.prototype,"_config",2);Xr=Ql([x("hui-link-card")],Xr);const f1=-60,g1=12,m1=1;function od(e,t=0){var r,o;if(typeof(e==null?void 0:e.state)=="number")return e.state;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.value)??((o=e==null?void 0:e.attributes)==null?void 0:o.gain);return typeof i=="number"?i:i!=null&&Number(i)||t}function b1(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.min)??((o=e==null?void 0:e.attributes)==null?void 0:o.min_value);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:f1}function v1(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.max)??((o=e==null?void 0:e.attributes)==null?void 0:o.max_value);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:g1}function _1(e,t){var r;const i=(r=e==null?void 0:e.attributes)==null?void 0:r.step;return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:m1}function y1(e){return e?"静音":"开启"}function x1(e){if(e>=1)return 0;const t=String(e),i=t.indexOf(".");return i>=0?t.length-i-1:0}var w1=Object.defineProperty,$1=Object.getOwnPropertyDescriptor,Gs=(e,t,i,r)=>{for(var o=r>1?void 0:r?$1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&w1(t,i,o),o};function ua(e,t){if(e==null||e==="")return t;const i=Number(e);return Number.isNaN(i)?t:i}let Qi=class extends y{constructor(){super(...arguments),this._muted=!1}static getStubConfig(){return{type:"gain",entity:"/demo/gain",name:"输入增益",icon:"mdi:volume-high",min:-60,max:12,step:1}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_gainState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}_range(){var t,i,r;const e=this._gainState();return{min:b1(e,ua((t=this._config)==null?void 0:t.min,-60)),max:v1(e,ua((i=this._config)==null?void 0:i.max,12)),step:_1(e,ua((r=this._config)==null?void 0:r.step,1))}}async _onGainChange(e){var i;if(e.stopPropagation(),this._muted)return;const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,e.detail.value)}async _toggleMute(e){var o;e.stopPropagation();const t=(o=this._config)==null?void 0:o.entity;if(!t||!this.flow)return;const{min:i}=this._range();if(this._muted){const n=this._savedGain??i;this._muted=!1,this._savedGain=void 0,await this.flow.callService(t,n);return}const r=od(this._gainState(),i);this._savedGain=r,this._muted=!0,await this.flow.callService(t,i)}render(){var C,O;if(!this._config)return m;const e=this._config.entity,t=this._gainState(),i=this._muted,r=!i,o=!!this._config.vertical,n=String(this._config.name??((C=t==null?void 0:t.attributes)==null?void 0:C.friendly_name)??e??"增益"),s=String(this._config.icon??((O=t==null?void 0:t.attributes)==null?void 0:O.icon)??"mdi:volume-high"),{min:a,max:l,step:c}=this._range(),d=od(t,a),p=i?a:d,f=x1(c),b=i?"mdi:volume-off":"mdi:volume-high",v=hr(r,this._config.color,"primary"),_=P({"--tile-color":v}),k=P({"--feature-color":v??(r?"var(--primary-color)":"var(--state-inactive-color)")});return h`
      <ha-card class=${T({active:r,muted:i})} style=${_}>
        <ha-tile-container ?vertical=${o} ?fixed-info-height=${o}>
          <ha-tile-icon slot="icon" .icon=${s} .active=${r}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${n}
            .secondary=${y1(i)}
          ></ha-tile-info>
          <div slot="features" class="feature-row" style=${k}>
            <ha-control-number-buttons
              class="gain-stepper"
              .value=${p}
              .min=${a}
              .max=${l}
              .step=${c}
              .disabled=${i}
              unit="dB"
              .digits=${f}
              @value-changed=${this._onGainChange}
              @click=${I=>I.stopPropagation()}
            ></ha-control-number-buttons>
            <button
              type="button"
              class="mute-btn"
              aria-pressed=${i?"true":"false"}
              aria-label=${i?"取消静音":"静音"}
              title=${i?"取消静音":"静音"}
              @click=${this._toggleMute}
            >
              <ha-icon .icon=${b}></ha-icon>
            </button>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Qi.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature-row {
        display: flex;
        align-items: stretch;
        gap: 8px;
        box-sizing: border-box;
      }
      .gain-stepper {
        flex: 1;
        min-width: 0;
      }
      .mute-btn {
        flex: 0 0 var(--feature-height);
        width: var(--feature-height);
        height: var(--feature-height);
        appearance: none;
        border: none;
        border-radius: var(--feature-border-radius);
        background: color-mix(
          in srgb,
          var(--feature-color, var(--primary-color)) 20%,
          transparent
        );
        color: var(--feature-color, var(--primary-color));
        display: flex;
        align-items: center;
        justify-content: center;
        cursor: pointer;
        padding: 0;
        --mdc-icon-size: 22px;
      }
      .mute-btn ha-icon {
        display: flex;
        align-items: center;
        justify-content: center;
        width: var(--mdc-icon-size);
        height: var(--mdc-icon-size);
        color: inherit;
      }
      .mute-btn:hover {
        background: color-mix(
          in srgb,
          var(--feature-color, var(--primary-color)) 32%,
          transparent
        );
      }
      ha-card.muted .mute-btn {
        color: var(--state-inactive-color);
        --feature-color: var(--state-inactive-color);
      }
    `];Gs([u({attribute:!1})],Qi.prototype,"flow",2);Gs([g()],Qi.prototype,"_config",2);Gs([g()],Qi.prototype,"_muted",2);Qi=Gs([x("hui-gain-card")],Qi);function W(e){return Math.min(1,Math.max(0,e))}function Qt(e,t){const i=Number(e);return Number.isNaN(i)?0:W(t?i/255:i)}function k1(e){const t=e.trim().replace(/^#/,"");if(!/^[0-9a-f]{3,8}$/i.test(t))return;const i=l=>l+l;let r=t;if((r.length===3||r.length===4)&&(r=[...r].map(i).join("")),r.length!==6&&r.length!==8)return;const o=Number.parseInt(r.slice(0,2),16)/255,n=Number.parseInt(r.slice(2,4),16)/255,s=Number.parseInt(r.slice(4,6),16)/255,a=r.length===8?Number.parseInt(r.slice(6,8),16)/255:1;return[o,n,s,a]}function Jl(e){if(Array.isArray(e)){const t=e.map(r=>Number(r));if(t.length<3||t.some(r=>Number.isNaN(r)))return;const i=t.some(r=>r>1);return[Qt(t[0],i),Qt(t[1],i),Qt(t[2],i),Qt(t.length>=4?t[3]:1,i)]}if(typeof e=="string"&&e.trim())return k1(e.trim());if(e&&typeof e=="object"){const t=e,i=t.r??t.red??t.x,r=t.g??t.green??t.y,o=t.b??t.blue??t.z;if(i!==void 0&&r!==void 0&&o!==void 0){const n=[i,r,o,t.a??t.alpha??t.w].map(s=>Number(s)).some(s=>!Number.isNaN(s)&&s>1);return[Qt(Number(i),n),Qt(Number(r),n),Qt(Number(o),n),Qt(Number(t.a??t.alpha??t.w??1),n)]}}}function Wa(e){const[t,i,r,o]=e,n=Math.round(t*255),s=Math.round(i*255),a=Math.round(r*255);return o>=.999?`rgb(${n}, ${s}, ${a})`:`rgba(${n}, ${s}, ${a}, ${o.toFixed(3)})`}function C1(e,t=!1){const i=o=>Math.round(W(o)*255).toString(16).padStart(2,"0"),r=`#${i(e[0])}${i(e[1])}${i(e[2])}`;return t?`${r}${i(e[3])}`:r}function fp(e){return e.map(t=>Number(t.toFixed(4)))}function gp(e){return C1(e,e[3]<.999).toUpperCase()}function S1(e){return .299*e[0]+.587*e[1]+.114*e[2]>.58?"#1e293b":"#ffffff"}function Ya(e){const[t,i,r,o]=e.map(W),n=Math.max(t,i,r),s=Math.min(t,i,r),a=n-s;let l=0;a>1e-6&&(n===t?l=(i-r)/a%6:n===i?l=(r-t)/a+2:l=(t-i)/a+4,l*=60,l<0&&(l+=360));const c=n<=1e-6?0:a/n;return{h:l,s:c,v:n,a:o}}function Za(e){const t=(e.h%360+360)%360,i=W(e.s),r=W(e.v),o=W(e.a),n=r*i,s=n*(1-Math.abs(t/60%2-1)),a=r-n;let l=0,c=0,d=0;return t<60?[l,c,d]=[n,s,0]:t<120?[l,c,d]=[s,n,0]:t<180?[l,c,d]=[0,n,s]:t<240?[l,c,d]=[0,s,n]:t<300?[l,c,d]=[s,0,n]:[l,c,d]=[n,0,s],[W(l+a),W(c+a),W(d+a),o]}function ec(e){if(Array.isArray(e)){if(e.length<3)return;const t=Number(e[0]),i=Number(e[1]),r=Number(e[2]);return[t,i,r].some(n=>Number.isNaN(n))?void 0:{h:t>1?(t%360+360)%360:W(t)*360,s:W(i),v:W(r),a:e.length>=4?W(Number(e[3])):1}}if(e&&typeof e=="object"){const t=e,i=Number(t.h??t.hue??t.H),r=Number(t.s??t.saturation??t.S),o=Number(t.v??t.value??t.V);return[i,r,o].some(s=>Number.isNaN(s))?void 0:{h:i>1?(i%360+360)%360:W(i)*360,s:W(r),v:W(o),a:W(Number(t.a??t.alpha??1))}}}function mp(e){return[Number(W(e.h/360).toFixed(6)),Number(W(e.s).toFixed(6)),Number(W(e.v).toFixed(6))]}function bp(e){const t=Math.round((e.h%360+360)%360),i=Math.round(W(e.s)*100),r=Math.round(W(e.v)*100);return`${t}° ${i}% ${r}%`}const nd=[.12,.53,.9,1];var E1=Object.defineProperty,P1=Object.getOwnPropertyDescriptor,xi=(e,t,i,r)=>{for(var o=r>1?void 0:r?P1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&E1(t,i,o),o};let bt=class extends y{constructor(){super(...arguments),this.hue=200,this.saturation=.8,this.value=1,this.alpha=1,this.disabled=!1,this._pressed=!1,this._onPointerDown=e=>{var t;this.disabled||(e.preventDefault(),this._pressed=!0,this._pointerId=e.pointerId,(t=this._pad)==null||t.setPointerCapture(e.pointerId),this._setFromPointer(e.clientX,e.clientY))},this._onPointerMove=e=>{!this._pressed||e.pointerId!==this._pointerId||this._setFromPointer(e.clientX,e.clientY)},this._onPointerUp=e=>{var t;if(e.pointerId===this._pointerId){this._pressed=!1,this._pointerId=void 0;try{(t=this._pad)==null||t.releasePointerCapture(e.pointerId)}catch{}}},this._onKeyDown=e=>{if(this.disabled)return;const t=2,i=.02;let r=this.hue,o=this.saturation;switch(e.key){case"ArrowRight":r+=t;break;case"ArrowLeft":r-=t;break;case"ArrowUp":o+=i;break;case"ArrowDown":o-=i;break;default:return}e.preventDefault(),this.hue=(r%360+360)%360,this.saturation=Math.max(0,Math.min(1,o)),this._emitChange()}}_hsva(){return{h:this.hue,s:this.saturation,v:this.value,a:this.alpha}}_rgba(){return Za(this._hsva())}_emitChange(){const e=this._rgba();this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{rgba:e,hsva:this._hsva()}}))}_setFromPointer(e,t){const i=this._pad;if(!i)return;const r=i.getBoundingClientRect();if(r.width<=0||r.height<=0)return;const o=Math.max(0,Math.min(1,(e-r.left)/r.width)),n=Math.max(0,Math.min(1,(t-r.top)/r.height));this.hue=Math.round(o*360*10)/10,this.saturation=Math.round((1-n)*1e3)/1e3,this._emitChange()}_handleStyle(){return{left:`${this.hue/360*100}%`,top:`${(1-this.saturation)*100}%`,background:Wa(this._rgba())}}applyRgba(e){const t=Ya(e);this.hue=t.h,this.saturation=t.s,this.value=t.v,this.alpha=t.a}render(){return h`
      <div
        class="pad"
        role="application"
        tabindex=${this.disabled?-1:0}
        aria-label="颜色触控板"
        aria-valuetext=${Wa(this._rgba())}
        aria-disabled=${this.disabled?"true":"false"}
        @pointerdown=${this._onPointerDown}
        @pointermove=${this._onPointerMove}
        @pointerup=${this._onPointerUp}
        @pointercancel=${this._onPointerUp}
        @keydown=${this._onKeyDown}
      >
        <div class="spectrum" aria-hidden="true"></div>
        <div class="sat-overlay" aria-hidden="true"></div>
        <div
          class="handle ${this._pressed?"pressed":""}"
          style=${P(this._handleStyle())}
          aria-hidden="true"
        ></div>
      </div>
    `}};bt.styles=w`
    :host {
      display: block;
      width: 100%;
      height: 100%;
      min-height: 120px;
      --control-color-border-radius: var(--ha-card-border-radius, 12px);
      --control-color-handle-size: 22px;
      touch-action: none;
      user-select: none;
    }
    :host([disabled]) {
      opacity: 0.45;
      pointer-events: none;
    }
    .pad {
      position: relative;
      width: 100%;
      height: 100%;
      min-height: inherit;
      border-radius: var(--control-color-border-radius);
      overflow: hidden;
      outline: none;
      cursor: crosshair;
      border: 1px solid rgba(0, 0, 0, 0.08);
      box-sizing: border-box;
    }
    .pad:focus-visible {
      box-shadow: 0 0 0 2px var(--primary-color, #03a9f4);
    }
    .spectrum {
      position: absolute;
      inset: 0;
      background: linear-gradient(
        to right,
        hsl(0, 100%, 50%),
        hsl(60, 100%, 50%),
        hsl(120, 100%, 50%),
        hsl(180, 100%, 50%),
        hsl(240, 100%, 50%),
        hsl(300, 100%, 50%),
        hsl(360, 100%, 50%)
      );
    }
    .sat-overlay {
      position: absolute;
      inset: 0;
      background: linear-gradient(to top, #fff, transparent);
      pointer-events: none;
    }
    .handle {
      position: absolute;
      width: var(--control-color-handle-size);
      height: var(--control-color-handle-size);
      margin-left: calc(var(--control-color-handle-size) / -2);
      margin-top: calc(var(--control-color-handle-size) / -2);
      border-radius: 50%;
      box-shadow:
        0 0 0 2px #fff,
        0 2px 8px rgba(0, 0, 0, 0.28);
      pointer-events: none;
      transition: transform 80ms ease-out;
      z-index: 1;
    }
    .handle.pressed {
      transform: scale(1.12);
    }
  `;xi([u({type:Number})],bt.prototype,"hue",2);xi([u({type:Number})],bt.prototype,"saturation",2);xi([u({type:Number})],bt.prototype,"value",2);xi([u({type:Number})],bt.prototype,"alpha",2);xi([u({type:Boolean,reflect:!0})],bt.prototype,"disabled",2);xi([g()],bt.prototype,"_pressed",2);xi([mi(".pad")],bt.prototype,"_pad",2);bt=xi([x("ha-control-color-pad")],bt);var A1=Object.defineProperty,tc=(e,t,i,r)=>{for(var o=void 0,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=s(t,i,o)||o);return o&&A1(t,i,o),o};const Sc=class Sc extends y{setConfig(t){this._config=t,this._localHsva=void 0,this.requestUpdate()}getGridOptions(){var i;return{columns:6,rows:5,min_columns:!!((i=this._config)!=null&&i.vertical)?3:6,min_rows:5}}_entityState(){var i;const t=(i=this._config)==null?void 0:i.entity;if(!(!t||!this.flow))return this.flow.states[t]}_remoteHsva(){var i;const t=(i=this._entityState())==null?void 0:i.state;return this.colorMode==="hsv"?ec(t)??Ya(nd):Ya(Jl(t)??nd)}_hsva(){return this._localHsva??this._remoteHsva()}_rgba(){return Za(this._hsva())}async _send(t){var r;const i=(r=this._config)==null?void 0:r.entity;if(!(!i||!this.flow)){if(this.colorMode==="hsv"){await this.flow.callService(i,mp(t));return}await this.flow.callService(i,fp(Za(t)))}}async _onPadChanged(t){t.stopPropagation(),this._localHsva=t.detail.hsva,await this._send(t.detail.hsva)}_onPadPointerUp(){window.setTimeout(()=>{this._localHsva=void 0},250)}async _onValueChange(t){t.stopPropagation();const i={...this._hsva(),v:t.detail.value};this._localHsva=i,await this._send(i)}async _onAlphaChange(t){t.stopPropagation();const i={...this._hsva(),a:t.detail.value};this._localHsva=i,await this._send(i)}render(){var _,k;if(!this._config)return m;const t=this._config.entity,i=this._entityState(),r=this._hsva(),o=this._rgba(),n=!!this._config.vertical,s=this.colorMode==="rgba",a=String(this._config.name??((_=i==null?void 0:i.attributes)==null?void 0:_.friendly_name)??t??this.stubName),l=String(this._config.icon??((k=i==null?void 0:i.attributes)==null?void 0:k.icon)??this.stubIcon),c=Wa(o),d=this.colorMode==="hsv"?bp(r):gp(o),p=S1(o),f=P({"--tile-color":c}),b=P({"--tile-icon-color":c,"--tile-icon-foreground":p,"--tile-icon-opacity":"1"}),v=n?P({"--ha-tile-info-gap":"4px","--ha-tile-info-min-height":"auto","--ha-tile-info-primary-min-height":"auto","--ha-tile-info-primary-line-height":"1.3"}):m;return h`
      <ha-card class=${T({active:!0})} style=${f}>
        <ha-tile-container ?vertical=${n} expand-features style=${v}>
          <ha-tile-icon
            slot="icon"
            style=${b}
            .icon=${l}
            .active=${!0}
          ></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${a} .secondary=${d}></ha-tile-info>
          <div slot="features" class="features">
            <ha-control-color-pad
              .hue=${r.h}
              .saturation=${r.s}
              .value=${r.v}
              .alpha=${r.a}
              @value-changed=${this._onPadChanged}
              @pointerup=${this._onPadPointerUp}
              @click=${C=>C.stopPropagation()}
            ></ha-control-color-pad>
            <div class="sliders">
              ${s?h`
                    <div class="slider-row">
                      <span class="slider-label">透明度</span>
                      <ha-control-slider
                        .value=${r.a}
                        .min=${0}
                        .max=${1}
                        .step=${.01}
                        show-handle
                        @value-changed=${this._onAlphaChange}
                        @click=${C=>C.stopPropagation()}
                      ></ha-control-slider>
                    </div>
                  `:h`
                    <div class="slider-row">
                      <span class="slider-label">明度</span>
                      <ha-control-slider
                        .value=${r.v}
                        .min=${0}
                        .max=${1}
                        .step=${.01}
                        show-handle
                        @value-changed=${this._onValueChange}
                        @click=${C=>C.stopPropagation()}
                      ></ha-control-slider>
                    </div>
                  `}
            </div>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Sc.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      .features {
        display: flex;
        flex-direction: column;
        gap: 8px;
        flex: 1 1 auto;
        min-height: 0;
        height: 100%;
        overflow: hidden;
        box-sizing: border-box;
      }
      ha-control-color-pad {
        flex: 1 1 120px;
        width: 100%;
        min-height: 120px;
        align-self: stretch;
      }
      .sliders {
        display: flex;
        flex-direction: column;
        gap: 4px;
        flex: 0 0 auto;
        --feature-height: 24px;
        --feature-border-radius: 8px;
      }
      .slider-row {
        display: grid;
        grid-template-columns: 3em 1fr;
        align-items: center;
        gap: 8px;
        min-height: var(--feature-height);
      }
      .slider-label {
        font-size: 11px;
        line-height: 1.2;
        color: var(--secondary-text-color);
        white-space: nowrap;
      }
    `];let Ji=Sc;tc([u({attribute:!1})],Ji.prototype,"flow");tc([g()],Ji.prototype,"_config");tc([g()],Ji.prototype,"_localHsva");var T1=Object.getOwnPropertyDescriptor,vp=(e,t,i,r)=>{for(var o=r>1?void 0:r?T1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=s(o)||o);return o};let Jo=class extends Ji{constructor(){super(...arguments),this.colorMode="rgba",this.stubEntity="/demo/rgba",this.stubName="RGBA",this.stubIcon="mdi:palette"}static getStubConfig(){return{type:"rgba",entity:"/demo/rgba",name:"RGBA",icon:"mdi:palette"}}};Jo=vp([x("hui-rgba-card")],Jo);let Ka=class extends Jo{static getStubConfig(){return{...Jo.getStubConfig(),type:"color"}}};Ka=vp([x("hui-color-card")],Ka);var M1=Object.getOwnPropertyDescriptor,O1=(e,t,i,r)=>{for(var o=r>1?void 0:r?M1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=s(o)||o);return o};let Qa=class extends Ji{constructor(){super(...arguments),this.colorMode="hsv",this.stubEntity="/demo/hsv",this.stubName="HSV",this.stubIcon="mdi:palette-swatch"}static getStubConfig(){return{type:"hsv",entity:"/demo/hsv",name:"HSV",icon:"mdi:palette-swatch"}}};Qa=O1([x("hui-hsv-card")],Qa);const Lt={count:4,min:0,max:1,step:.01,max_count:16};function sd(e,t,i){return Math.min(i,Math.max(t,e))}function ad(e,t=Lt.count){const i=Number(e);return Number.isFinite(i)?Math.max(1,Math.min(Lt.max_count,Math.round(i))):t}function Ja(e,t,i,r){var s;if(!Number.isFinite(t)||t<=0)return sd(e,i,r);const o=Math.round(e/t)*t,n=String(t).includes(".")?((s=String(t).split(".")[1])==null?void 0:s.length)??0:0;return sd(Number(o.toFixed(Math.min(n+2,8))),i,r)}function L1(e,t){let i=[];return Array.isArray(e)?i=e.map(r=>String(r??"").trim()):typeof e=="string"&&e.trim()&&(i=e.split(/[,???|]/).map(r=>r.trim())),Array.from({length:t},(r,o)=>i[o]||String(o))}function _p(e,t,i){const r=i.fill??Ja((i.min+i.max)/2,i.step,i.min,i.max),o=Array.from({length:t},()=>r);let n;if(Array.isArray(e))n=e.map(s=>Number(s));else if(typeof e=="string"&&e.trim())try{const s=JSON.parse(e);Array.isArray(s)&&(n=s.map(a=>Number(a)))}catch{const s=e.split(/[,;\s]+/).map(a=>Number(a.trim()));s.length&&!s.some(a=>Number.isNaN(a))&&(n=s)}if(!n)return o;for(let s=0;s<t;s+=1){const a=n[s];o[s]=Number.isFinite(a)?Ja(a,i.step,i.min,i.max):r}return o}function yp(e){return e.map(t=>Number(t.toFixed(6)))}function D1(e,t=2){return e.map(i=>i.toFixed(t)).join(", ")}var I1=Object.defineProperty,H1=Object.getOwnPropertyDescriptor,Xs=(e,t,i,r)=>{for(var o=r>1?void 0:r?H1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&I1(t,i,o),o};function fa(e,t){if(e==null||e==="")return t;const i=Number(e);return Number.isNaN(i)?t:i}let er=class extends y{static getStubConfig(){return{type:"multi-fader",entity:"/demo/faders",name:"多维推杆",icon:"mdi:tune-vertical-variant",count:Lt.count,min:Lt.min,max:Lt.max,step:Lt.step,labels:"0,1,2,3",orientation:"vertical"}}setConfig(e){this._config=e,this._local=void 0,this.requestUpdate()}getGridOptions(){var o,n;const e=!!((o=this._config)!=null&&o.vertical),t=ad((n=this._config)==null?void 0:n.count);if(this._orientation()==="vertical")return{columns:Math.min(12,Math.max(6,t*2)),rows:4,min_columns:e?3:6,min_rows:4};const r=Math.max(2,Math.min(8,1+Math.ceil(t/2)));return{columns:6,rows:r,min_columns:e?3:6,min_rows:r}}_orientation(){var e;return((e=this._config)==null?void 0:e.orientation)==="horizontal"?"horizontal":"vertical"}_options(){var e,t,i,r;return{count:ad((e=this._config)==null?void 0:e.count),min:fa((t=this._config)==null?void 0:t.min,Lt.min),max:fa((i=this._config)==null?void 0:i.max,Lt.max),step:fa((r=this._config)==null?void 0:r.step,Lt.step)}}_values(){var s,a;const{count:e,min:t,max:i,step:r}=this._options();if(this._local&&this._local.length===e)return this._local;const o=(s=this._config)==null?void 0:s.entity,n=o&&this.flow?(a=this.flow.states[o])==null?void 0:a.state:void 0;return _p(n,e,{min:t,max:i,step:r})}async _send(e){var i;const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,yp(e))}async _onFaderChange(e,t){t.stopPropagation();const{count:i,min:r,max:o,step:n}=this._options(),s=[...this._values()];for(;s.length<i;)s.push(r);s[e]=Ja(t.detail.value,n,r,o),this._local=s.slice(0,i),await this._send(this._local)}_onPointerUp(){window.setTimeout(()=>{this._local=void 0},250)}render(){var I,V;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,{count:i,min:r,max:o,step:n}=this._options(),s=this._values(),a=L1(this._config.labels,i),l=this._orientation(),c=!!this._config.vertical,d=String(this._config.name??((I=t==null?void 0:t.attributes)==null?void 0:I.friendly_name)??e??"多维推杆"),p=String(this._config.icon??((V=t==null?void 0:t.attributes)==null?void 0:V.icon)??"mdi:tune-vertical-variant"),f=s.some(K=>K>r),b=hr(f,this._config.color,"primary"),v=P({"--tile-color":b}),_=P({"--feature-color":b??"var(--primary-color)"}),k=n<1?2:0,O=!!this._config.hide_state?"":D1(s,k);return h`
      <ha-card class=${T({active:f})} style=${v}>
        <ha-tile-container ?vertical=${c} expand-features>
          <ha-tile-icon slot="icon" .icon=${p} .active=${f}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${d}
            .secondary=${O}
          ></ha-tile-info>
          <div
            slot="features"
            class=${T({faders:!0,vertical:l==="vertical",horizontal:l==="horizontal"})}
            style=${_}
          >
            ${s.map((K,R)=>h`
                <div class="fader">
                  ${l==="horizontal"?h`<span class="fader-label">${a[R]}</span>`:m}
                  <ha-control-slider
                    .value=${K}
                    .min=${r}
                    .max=${o}
                    .step=${n}
                    ?vertical=${l==="vertical"}
                    show-handle
                    .roundValue=${n>=1}
                    @value-changed=${Q=>this._onFaderChange(R,Q)}
                    @pointerup=${this._onPointerUp}
                    @click=${Q=>Q.stopPropagation()}
                  ></ha-control-slider>
                  ${l==="vertical"?h`<span class="fader-label">${a[R]}</span>`:m}
                </div>
              `)}
          </div>
        </ha-tile-container>
      </ha-card>
    `}};er.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
        /* 竖向推杆轨道最小高度（自动高度 / 弹窗无也要可见） */
        --multi-fader-track-min-height: 96px;
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      .faders {
        display: flex;
        box-sizing: border-box;
        min-height: 0;
        height: 100%;
        /* 与 tile features 默认内边距一致，避免贴边 */
        padding: 0 12px 12px;
        --feature-height: 28px;
        --feature-border-radius: 10px;
      }
      .faders.vertical {
        flex-direction: row;
        align-items: stretch;
        justify-content: space-evenly;
        gap: 6px;
        --feature-height: 28px;
        padding: 4px 10px 14px !important;
        /*
          ha-tile-container[expand-features] 的 ::slotted 会把 min-height 设为 0，
          自动高度时竖向推杆会被压没；用 !important 保证最小轨道高度。
          预留：轨道 + 标签 + 底边距。
         */
        min-height: calc(var(--multi-fader-track-min-height) + 36px) !important;
      }
      .faders.horizontal {
        flex-direction: column;
        gap: 6px;
        --feature-height: 24px;
      }
      .fader {
        display: flex;
        min-width: 0;
        min-height: 0;
      }
      .faders.vertical .fader {
        flex: 1 1 0;
        flex-direction: column;
        align-items: center;
        gap: 6px;
        min-height: 0;
      }
      .faders.vertical ha-control-slider {
        flex: 1 1 auto;
        min-height: var(--multi-fader-track-min-height);
        width: var(--feature-height);
      }
      .faders.horizontal .fader {
        flex-direction: row;
        align-items: center;
        gap: 8px;
      }
      .faders.horizontal ha-control-slider {
        flex: 1 1 auto;
        min-width: 0;
      }
      .fader-label {
        flex: 0 0 auto;
        font-size: 11px;
        line-height: 1.2;
        color: var(--secondary-text-color);
        text-align: center;
        white-space: nowrap;
        max-width: 100%;
        overflow: hidden;
        text-overflow: ellipsis;
      }
      .faders.horizontal .fader-label {
        width: 2.5em;
        text-align: left;
      }
    `];Xs([u({attribute:!1})],er.prototype,"flow",2);Xs([g()],er.prototype,"_config",2);Xs([g()],er.prototype,"_local",2);er=Xs([x("hui-multi-fader-card")],er);const j={x_min:0,x_max:1,y_min:0,y_max:1,step:.01,invert_y:!1,snap_center:!1,show_grid:!0};function ld(e,t,i){return Math.min(i,Math.max(t,e))}function cd(e,t,i,r){var a;if(!Number.isFinite(t)||t<=0)return ld(e,i,r);const o=Math.round(e/t)*t,n=String(t).includes(".")?((a=String(t).split(".")[1])==null?void 0:a.length)??0:0,s=Number(o.toFixed(Math.min(n+2,8)));return ld(s,i,r)}function ic(e){if(Array.isArray(e)){if(e.length<2)return;const t=Number(e[0]),i=Number(e[1]);return Number.isNaN(t)||Number.isNaN(i)?void 0:{x:t,y:i}}if(e&&typeof e=="object"){const t=e,i=Number(t.x??t.X??t.left),r=Number(t.y??t.Y??t.top);return Number.isNaN(i)||Number.isNaN(r)?void 0:{x:i,y:r}}if(typeof e=="string"&&e.trim())try{const t=JSON.parse(e);return ic(t)}catch{const t=e.split(/[,;\s]+/).map(i=>Number(i.trim()));if(t.length>=2&&!t.some(i=>Number.isNaN(i)))return{x:t[0],y:t[1]}}}function xp(e){return[Number(e.x.toFixed(6)),Number(e.y.toFixed(6))]}function z1(e,t=2){return`${e.x.toFixed(t)}, ${e.y.toFixed(t)}`}function V1(e,t){return{x:cd(e.x,t.step,t.xMin,t.xMax),y:cd(e.y,t.step,t.yMin,t.yMax)}}function el(e,t,i,r){return{x:(e+t)/2,y:(i+r)/2}}var N1=Object.defineProperty,R1=Object.getOwnPropertyDescriptor,Ee=(e,t,i,r)=>{for(var o=r>1?void 0:r?R1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&N1(t,i,o),o};let ce=class extends y{constructor(){super(...arguments),this.x=0,this.y=0,this.xMin=0,this.xMax=1,this.yMin=0,this.yMax=1,this.step=.01,this.invertY=!1,this.showGrid=!0,this.snapCenter=!1,this.disabled=!1,this._pressed=!1,this._onPointerDown=e=>{var t;this.disabled||(e.preventDefault(),this._pressed=!0,this._pointerId=e.pointerId,(t=this._pad)==null||t.setPointerCapture(e.pointerId),this._setFromPointer(e.clientX,e.clientY))},this._onPointerMove=e=>{!this._pressed||e.pointerId!==this._pointerId||this._setFromPointer(e.clientX,e.clientY)},this._onPointerUp=e=>{var t;if(e.pointerId===this._pointerId){this._pressed=!1,this._pointerId=void 0;try{(t=this._pad)==null||t.releasePointerCapture(e.pointerId)}catch{}if(this.snapCenter){const i=this._normalize(el(this.xMin,this.xMax,this.yMin,this.yMax));this.x=i.x,this.y=i.y,this._emitChange(i)}this.dispatchEvent(new CustomEvent("drag-end",{bubbles:!0,composed:!0,detail:{x:this.x,y:this.y}}))}},this._onKeyDown=e=>{if(this.disabled)return;let t=this.x,i=this.y;const r=this.step>0?this.step:.01;switch(e.key){case"ArrowRight":t+=r;break;case"ArrowLeft":t-=r;break;case"ArrowUp":i+=this.invertY?-r:r;break;case"ArrowDown":i+=this.invertY?r:-r;break;case"Home":t=this.xMin,i=this.yMin;break;case"End":t=this.xMax,i=this.yMax;break;default:return}e.preventDefault();const o=this._normalize({x:t,y:i});this.x=o.x,this.y=o.y,this._emitChange(o)}}_emitChange(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:e}))}_normalize(e){return V1(e,{xMin:this.xMin,xMax:this.xMax,yMin:this.yMin,yMax:this.yMax,step:this.step})}_setFromPointer(e,t){const i=this._pad;if(!i)return;const r=i.getBoundingClientRect();if(r.width<=0||r.height<=0)return;const o=Math.max(0,Math.min(1,(e-r.left)/r.width)),n=Math.max(0,Math.min(1,(t-r.top)/r.height)),s=this.xMin+o*(this.xMax-this.xMin),a=this.invertY?n:1-n,l=this.yMin+a*(this.yMax-this.yMin),c=this._normalize({x:s,y:l});this.x=c.x,this.y=c.y,this._emitChange(c)}_handleStyle(){const e=this.xMax-this.xMin||1,t=this.yMax-this.yMin||1,i=(this.x-this.xMin)/e*100,r=(this.y-this.yMin)/t,o=(this.invertY?r:1-r)*100;return{left:`${i}%`,top:`${o}%`}}render(){return h`
      <div
        class="pad"
        role="application"
        tabindex=${this.disabled?-1:0}
        aria-label="XY 触控板"
        aria-valuetext=${`${this.x}, ${this.y}`}
        aria-disabled=${this.disabled?"true":"false"}
        @pointerdown=${this._onPointerDown}
        @pointermove=${this._onPointerMove}
        @pointerup=${this._onPointerUp}
        @pointercancel=${this._onPointerUp}
        @keydown=${this._onKeyDown}
      >
        ${this.showGrid?h`
              <div class="grid" aria-hidden="true">
                <span class="hline"></span>
                <span class="vline"></span>
              </div>
            `:null}
        <div
          class="handle ${this._pressed?"pressed":""}"
          style=${P(this._handleStyle())}
          aria-hidden="true"
        ></div>
      </div>
    `}};ce.styles=w`
    :host {
      display: block;
      width: 100%;
      height: 100%;
      min-height: 120px;
      --control-xy-color: var(--feature-color, var(--primary-color, #03a9f4));
      --control-xy-background: color-mix(
        in srgb,
        var(--control-xy-color) 12%,
        var(--secondary-background-color, #f5f5f5)
      );
      --control-xy-handle-size: 22px;
      --control-xy-border-radius: var(--ha-card-border-radius, 12px);
      touch-action: none;
      user-select: none;
    }
    :host([disabled]) {
      opacity: 0.45;
      pointer-events: none;
    }
    .pad {
      position: relative;
      width: 100%;
      height: 100%;
      min-height: inherit;
      border-radius: var(--control-xy-border-radius);
      background: var(--control-xy-background);
      box-sizing: border-box;
      outline: none;
      cursor: crosshair;
      overflow: hidden;
      border: 1px solid color-mix(in srgb, var(--control-xy-color) 25%, transparent);
    }
    .pad:focus-visible {
      box-shadow: 0 0 0 2px var(--control-xy-color);
    }
    .grid {
      position: absolute;
      inset: 0;
      pointer-events: none;
    }
    .hline,
    .vline {
      position: absolute;
      background: color-mix(in srgb, var(--control-xy-color) 28%, transparent);
    }
    .hline {
      left: 0;
      right: 0;
      top: 50%;
      height: 1px;
      transform: translateY(-0.5px);
    }
    .vline {
      top: 0;
      bottom: 0;
      left: 50%;
      width: 1px;
      transform: translateX(-0.5px);
    }
    .handle {
      position: absolute;
      width: var(--control-xy-handle-size);
      height: var(--control-xy-handle-size);
      margin-left: calc(var(--control-xy-handle-size) / -2);
      margin-top: calc(var(--control-xy-handle-size) / -2);
      border-radius: 50%;
      background: var(--control-xy-color);
      box-shadow:
        0 0 0 2px #fff,
        0 2px 8px rgba(0, 0, 0, 0.2);
      pointer-events: none;
      transition: transform 80ms ease-out;
    }
    .handle.pressed {
      transform: scale(1.12);
    }
  `;Ee([u({type:Number})],ce.prototype,"x",2);Ee([u({type:Number})],ce.prototype,"y",2);Ee([u({type:Number,attribute:"x-min"})],ce.prototype,"xMin",2);Ee([u({type:Number,attribute:"x-max"})],ce.prototype,"xMax",2);Ee([u({type:Number,attribute:"y-min"})],ce.prototype,"yMin",2);Ee([u({type:Number,attribute:"y-max"})],ce.prototype,"yMax",2);Ee([u({type:Number})],ce.prototype,"step",2);Ee([u({type:Boolean})],ce.prototype,"invertY",2);Ee([u({type:Boolean})],ce.prototype,"showGrid",2);Ee([u({type:Boolean})],ce.prototype,"snapCenter",2);Ee([u({type:Boolean,reflect:!0})],ce.prototype,"disabled",2);Ee([g()],ce.prototype,"_pressed",2);Ee([mi(".pad")],ce.prototype,"_pad",2);ce=Ee([x("ha-control-xy-pad")],ce);var F1=Object.defineProperty,B1=Object.getOwnPropertyDescriptor,Ws=(e,t,i,r)=>{for(var o=r>1?void 0:r?B1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&F1(t,i,o),o};function _o(e,t){if(e==null||e==="")return t;const i=Number(e);return Number.isNaN(i)?t:i}function ga(e,t){return e==null?t:!!e}let tr=class extends y{constructor(){super(...arguments),this._writeChain=Promise.resolve()}static getStubConfig(){return{type:"xy-pad",entity:"/demo/xy",name:"XY Pad",icon:"mdi:axis-arrow",x_min:j.x_min,x_max:j.x_max,y_min:j.y_min,y_max:j.y_max,step:j.step,invert_y:j.invert_y,show_grid:j.show_grid,snap_center:j.snap_center}}setConfig(e){this._config=e,this._local=void 0,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?4:3;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_ranges(){var e,t,i,r,o,n,s,a;return{xMin:_o((e=this._config)==null?void 0:e.x_min,j.x_min),xMax:_o((t=this._config)==null?void 0:t.x_max,j.x_max),yMin:_o((i=this._config)==null?void 0:i.y_min,j.y_min),yMax:_o((r=this._config)==null?void 0:r.y_max,j.y_max),step:_o((o=this._config)==null?void 0:o.step,j.step),invertY:ga((n=this._config)==null?void 0:n.invert_y,j.invert_y),showGrid:ga((s=this._config)==null?void 0:s.show_grid,j.show_grid),snapCenter:ga((a=this._config)==null?void 0:a.snap_center,j.snap_center)}}_entityX(){var t;const e=(t=this._config)==null?void 0:t.entity_x;if(typeof e=="string"&&e.trim())return e.trim()}_entityY(){var t;const e=(t=this._config)==null?void 0:t.entity_y;if(typeof e=="string"&&e.trim())return e.trim()}_usesDualEntities(){return!!(this._entityX()&&this._entityY())}_readRemotePoint(){var s,a,l,c;const{xMin:e,xMax:t,yMin:i,yMax:r}=this._ranges(),o=el(e,t,i,r);if(!this.flow)return o;if(this._usesDualEntities()){const d=this._entityX(),p=this._entityY(),f=(s=this.flow.states[d])==null?void 0:s.state,b=(a=this.flow.states[p])==null?void 0:a.state,v=typeof f=="number"?f:Number(f),_=typeof b=="number"?b:Number(b);return{x:Number.isNaN(v)?o.x:v,y:Number.isNaN(_)?o.y:_}}const n=(l=this._config)==null?void 0:l.entity;return n?ic((c=this.flow.states[n])==null?void 0:c.state)??o:o}_currentPoint(){return this._local??this._readRemotePoint()}_writePoint(e){const t=async()=>{var r;if(!this.flow)return;if(this._usesDualEntities()){const o=this._entityX(),n=this._entityY();await Promise.all([this.flow.callService(o,e.x),this.flow.callService(n,e.y)]);return}const i=(r=this._config)==null?void 0:r.entity;i&&await this.flow.callService(i,xp(e))};return this._writeChain=this._writeChain.then(t,t),this._writeChain}_scheduleReleaseLocal(e){this._releaseLocalTimer!==void 0&&window.clearTimeout(this._releaseLocalTimer),this._releaseLocalTimer=window.setTimeout(()=>{this._releaseLocalTimer=void 0,this._local=void 0},e)}async _onValueChanged(e){e.stopPropagation(),this._local=e.detail,await this._writePoint(e.detail)}async _onDragEnd(e){e.stopPropagation();const{snapCenter:t,xMin:i,xMax:r,yMin:o,yMax:n}=this._ranges();if(t){const s=el(i,r,o,n);this._local=s,await this._writePoint(s),this._scheduleReleaseLocal(500);return}this._scheduleReleaseLocal(250)}disconnectedCallback(){super.disconnectedCallback(),this._releaseLocalTimer!==void 0&&(window.clearTimeout(this._releaseLocalTimer),this._releaseLocalTimer=void 0)}render(){var b,v;if(!this._config)return m;const e=this._ranges(),t=this._currentPoint(),r=this._usesDualEntities()?this._entityX():this._config.entity,o=r&&this.flow?this.flow.states[r]:void 0,n=!!this._config.vertical,s=String(this._config.name??((b=o==null?void 0:o.attributes)==null?void 0:b.friendly_name)??r??"XY Pad"),a=String(this._config.icon??((v=o==null?void 0:o.attributes)==null?void 0:v.icon)??"mdi:axis-arrow"),l=z1(t),c=!0,d=hr(c,this._config.color,"primary"),p=P({"--tile-color":d}),f=P({"--feature-color":d??"var(--primary-color)"});return h`
      <ha-card class=${T({active:c})} style=${p}>
        <ha-tile-container ?vertical=${n} expand-features>
          <ha-tile-icon slot="icon" .icon=${a} .active=${c}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${s} .secondary=${l}></ha-tile-info>
          <div slot="features" class="feature" style=${f}>
            <ha-control-xy-pad
              .x=${t.x}
              .y=${t.y}
              .xMin=${e.xMin}
              .xMax=${e.xMax}
              .yMin=${e.yMin}
              .yMax=${e.yMax}
              .step=${e.step}
              .invertY=${e.invertY}
              .showGrid=${e.showGrid}
              .snapCenter=${e.snapCenter}
              @value-changed=${this._onValueChanged}
              @drag-end=${this._onDragEnd}
              @click=${_=>_.stopPropagation()}
            ></ha-control-xy-pad>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};tr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
        flex: 1 1 auto;
        min-height: 120px;
        height: 100%;
        display: flex;
        flex-direction: column;
      }
      ha-control-xy-pad {
        flex: 1 1 auto;
        width: 100%;
        min-height: 120px;
        height: 100%;
      }
    `];Ws([u({attribute:!1})],tr.prototype,"flow",2);Ws([g()],tr.prototype,"_config",2);Ws([g()],tr.prototype,"_local",2);tr=Ws([x("hui-xy-pad-card")],tr);const U1=new Set(["playing","play","on","true","running","started"]);function rc(e){if(!e)return!1;const t=e.state;return typeof t=="boolean"?t:typeof t=="number"?t>0:U1.has(String(t).toLowerCase())}function j1(e){return rc(e)?"play":"stop"}function q1(e){return rc(e)?"正在播放":"已停止"}function G1(e,t){const i=e==="play"?"play_value":"stop_value";return i in(t??{})?X1(t==null?void 0:t[i]):e==="play"}function X1(e){if(typeof e=="boolean"||typeof e=="number")return e;if(e==null||e==="")return!0;const t=String(e).trim();if(t==="true")return!0;if(t==="false")return!1;const i=Number(t);return!Number.isNaN(i)&&t!==""?i:t}var W1=Object.defineProperty,Y1=Object.getOwnPropertyDescriptor,oc=(e,t,i,r)=>{for(var o=r>1?void 0:r?Y1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&W1(t,i,o),o};let en=class extends y{constructor(){super(...arguments),this.value="stop",this.disabled=!1}_emit(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{action:e}}))}_select(e,t){t.stopPropagation(),!(this.disabled||this.value===e)&&(this.value=e,this._emit(e))}render(){return h`
      <div class="container" role="radiogroup" aria-label="播放控制">
        <button
          type="button"
          class=${T({segment:!0,selected:this.value==="play"})}
          role="radio"
          aria-checked=${this.value==="play"?"true":"false"}
          aria-label="播放"
          title="播放"
          ?disabled=${this.disabled}
          @click=${e=>this._select("play",e)}
        >
          <ha-icon .icon=${"mdi:play"}></ha-icon>
        </button>
        <button
          type="button"
          class=${T({segment:!0,selected:this.value==="stop"})}
          role="radio"
          aria-checked=${this.value==="stop"?"true":"false"}
          aria-label="停止"
          title="停止"
          ?disabled=${this.disabled}
          @click=${e=>this._select("stop",e)}
        >
          <ha-icon .icon=${"mdi:stop"}></ha-icon>
        </button>
      </div>
    `}};en.styles=w`
    :host {
      display: block;
      --control-color: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-background: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-background-opacity: 0.2;
      --control-height: var(--feature-height, 40px);
      --control-border-radius: var(
        --feature-border-radius,
        var(--ha-card-border-radius, var(--ha-border-radius-lg, 12px))
      );
      height: var(--control-height);
      width: 100%;
    }
    .container {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 8px;
      height: 100%;
      box-sizing: border-box;
    }
    .segment {
      appearance: none;
      border: none;
      border-radius: var(--control-border-radius);
      background: color-mix(
        in srgb,
        var(--control-background) calc(var(--control-background-opacity) * 100%),
        transparent
      );
      color: var(--primary-text-color);
      cursor: pointer;
      min-width: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 0;
      transition:
        background-color 180ms ease,
        color 180ms ease;
      --mdc-icon-size: 22px;
    }
    .segment ha-icon {
      display: flex;
      align-items: center;
      justify-content: center;
      width: var(--mdc-icon-size);
      height: var(--mdc-icon-size);
      color: inherit;
    }
    .segment.selected {
      background: var(--control-color);
      color: white;
    }
    .segment:hover:not(:disabled):not(.selected) {
      background: color-mix(in srgb, var(--control-color) 32%, transparent);
    }
    .segment:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
  `;oc([u({type:String})],en.prototype,"value",2);oc([u({type:Boolean})],en.prototype,"disabled",2);en=oc([x("ha-control-media-playback")],en);var Z1=Object.defineProperty,K1=Object.getOwnPropertyDescriptor,nc=(e,t,i,r)=>{for(var o=r>1?void 0:r?K1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Z1(t,i,o),o};let Wr=class extends y{static getStubConfig(){return{type:"media",entity:"/demo/media",name:"媒体播放器",icon:"mdi:cast-audio"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _onPlaybackChange(e){var r;e.stopPropagation();const t=(r=this._config)==null?void 0:r.entity;if(!t||!this.flow)return;const i=G1(e.detail.action,this._config);await this.flow.callService(t,i)}render(){var d,p;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=rc(t),r=j1(t),o=!!this._config.vertical,n=String(this._config.name??((d=t==null?void 0:t.attributes)==null?void 0:d.friendly_name)??e??"媒体播放器"),s=String(this._config.icon??((p=t==null?void 0:t.attributes)==null?void 0:p.icon)??"mdi:cast-audio"),a=hr(i,this._config.color,"primary"),l=P({"--tile-color":a}),c=P({"--feature-color":a??(i?"var(--primary-color)":"var(--state-inactive-color)")});return h`
      <ha-card class=${T({active:i})} style=${l}>
        <ha-tile-container ?vertical=${o} ?fixed-info-height=${o}>
          <ha-tile-icon slot="icon" .icon=${s} .active=${i}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${n}
            .secondary=${q1(t)}
          ></ha-tile-info>
          <div slot="features" class="feature" style=${c}>
            <ha-control-media-playback
              .value=${r}
              @value-changed=${this._onPlaybackChange}
              @click=${f=>f.stopPropagation()}
            ></ha-control-media-playback>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Wr.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
      }
    `];nc([u({attribute:!1})],Wr.prototype,"flow",2);nc([g()],Wr.prototype,"_config",2);Wr=nc([x("hui-media-card")],Wr);var Q1=Object.defineProperty,J1=Object.getOwnPropertyDescriptor,ao=(e,t,i,r)=>{for(var o=r>1?void 0:r?J1(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Q1(t,i,o),o};const tl=48e3,ko=960,dd=96*1024,ev=`
class PcmCaptureProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this._acc = new Float32Array(2048);
    this._fill = 0;
    this._target = ${ko};
    this.port.onmessage = (ev) => {
      const msg = ev.data;
      if (!msg || msg.type !== "flush") return;
      if (this._fill > 0) {
        const out = this._acc.slice(0, this._fill);
        this.port.postMessage(out, [out.buffer]);
        this._fill = 0;
      }
      this.port.postMessage({ type: "flushed" });
    };
  }
  process(inputs) {
    const input = inputs[0];
    if (!input || !input.length) return true;
    const ch0 = input[0];
    if (!ch0 || !ch0.length) return true;
    const n = ch0.length;
    const stereo = input.length > 1 && input[1] && input[1].length === n;
    const ch1 = stereo ? input[1] : null;
    let offset = 0;
    while (offset < n) {
      const space = this._target - this._fill;
      const take = Math.min(space, n - offset);
      for (let i = 0; i < take; i++) {
        const idx = offset + i;
        this._acc[this._fill + i] = stereo
          ? 0.5 * (ch0[idx] + ch1[idx])
          : ch0[idx];
      }
      this._fill += take;
      offset += take;
      if (this._fill >= this._target) {
        const out = this._acc.slice(0, this._target);
        this.port.postMessage(out, [out.buffer]);
        this._fill = 0;
      }
    }
    return true;
  }
}
registerProcessor("pcm-capture-processor", PcmCaptureProcessor);
`;function tv(e,t){if(!t||Math.abs(t-tl)<1)return e;const i=t/tl,r=Math.max(1,Math.floor(e.length/i)),o=new Float32Array(r);for(let n=0;n<r;n++){const s=n*i,a=Math.floor(s),l=Math.min(a+1,e.length-1),c=s-a;o[n]=e[a]*(1-c)+e[l]*c}return o}function hd(e){if(typeof e=="number"&&Number.isFinite(e)&&e>0)return Math.round(e);if(typeof e=="string"&&e.trim()){const t=Number(e);if(Number.isFinite(t)&&t>0)return Math.round(t)}}function ma(){const e=navigator;if(typeof window<"u"&&window.isSecureContext===!1)return{reason:"麦克风需 https 或 localhost（http://局域网IP 不可用）"};if(e.mediaDevices)return{getUserMedia:i=>e.mediaDevices.getUserMedia(i)};const t=e.webkitGetUserMedia||e.mozGetUserMedia||e.getUserMedia;return t?{getUserMedia:i=>new Promise((r,o)=>{t.call(e,i,r,o)})}:{reason:"浏览器未提供麦克风接口"}}let Bt=class extends y{constructor(){super(...arguments),this._ready=!1,this._connecting=!1,this._error="",this._pressActive=!1,this._pressToken=0,this._ws=null,this._mediaStream=null,this._audioCtx=null,this._workletNode=null,this._scriptNode=null,this._sourceNode=null,this._muteNode=null,this._workletUrl=null,this._sendAcc=new Float32Array(ko*2),this._sendFill=0,this._draining=!1,this._flushDone=null,this._stopChain=null,this._onWindowPointerUp=e=>{this._endPress(e.pointerId)}}static getStubConfig(){return{type:"mic",entity:"/demo/webmic/port",name:"对讲",icon:"mdi:microphone"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}connectedCallback(){super.connectedCallback();const e=ma();e.reason&&!this._error&&(this._error=e.reason)}disconnectedCallback(){this._unbindWindowRelease(),this._stopTalking(),super.disconnectedCallback()}_bindWindowRelease(){window.addEventListener("pointerup",this._onWindowPointerUp,!0),window.addEventListener("pointercancel",this._onWindowPointerUp,!0)}_unbindWindowRelease(){window.removeEventListener("pointerup",this._onWindowPointerUp,!0),window.removeEventListener("pointercancel",this._onWindowPointerUp,!0)}_portNumber(){var t,i,r;const e=(t=this._config)==null?void 0:t.entity;if(e&&this.flow){const o=hd((i=this.flow.states[e])==null?void 0:i.state);if(o!==void 0)return o}return hd((r=this._config)==null?void 0:r.port)}_wsHost(){var t;const e=(t=this._config)==null?void 0:t.host;return typeof e=="string"&&e.trim()?e.trim():location.hostname||"127.0.0.1"}async _startTalking(e){if(e.pointerType==="mouse"&&e.button!==0||(e.preventDefault(),e.stopPropagation(),this._pressActive))return;const t=ma();if(!t.getUserMedia){this._error=t.reason??"无法访问麦克风";return}const i=this._portNumber();if(!i){this._error="未绑定有效端口";return}this._pressActive=!0,this._pointerId=e.pointerId;const r=++this._pressToken;this._error="",this._ready=!1,this._connecting=!0,this._bindWindowRelease();try{e.currentTarget.setPointerCapture(e.pointerId)}catch{}try{if(await this._openStream(i,t.getUserMedia,r),!this._pressActive||r!==this._pressToken){await this._stopTalking();return}this._connecting=!1,this._ready=!0}catch(o){!(o instanceof Error&&o.message==="cancelled")&&r===this._pressToken&&(this._error=o instanceof Error?o.message:"无法开始对讲"),await this._stopTalking()}}async _endPress(e){this._pressActive&&(e!==void 0&&this._pointerId!==void 0&&e!==this._pointerId||(this._draining=!0,this._pressActive=!1,this._pressToken+=1,this._unbindWindowRelease(),await this._stopTalking()))}async _onPointerUp(e){e.preventDefault(),e.stopPropagation(),await this._endPress(e.pointerId)}_onContextMenu(e){e.preventDefault()}_handlePcm(e,t){if(!this._pressActive&&!this._draining||!this._ws||this._ws.readyState!==WebSocket.OPEN)return;if(!this._draining&&this._ws.bufferedAmount>dd){this._sendFill=0;return}const i=tv(e,t);if(!i.length)return;let r=0;for(;r<i.length;){const o=ko-this._sendFill,n=Math.min(o,i.length-r);if(this._sendFill+n>this._sendAcc.length){const s=new Float32Array(Math.max(this._sendAcc.length*2,this._sendFill+n));s.set(this._sendAcc.subarray(0,this._sendFill)),this._sendAcc=s}if(this._sendAcc.set(i.subarray(r,r+n),this._sendFill),this._sendFill+=n,r+=n,this._sendFill>=ko){if(!this._draining&&this._ws.bufferedAmount>dd){this._sendFill=0;return}const s=this._sendAcc.slice(0,ko);this._ws.send(s.buffer),this._sendFill=0}}}async _openStream(e,t,i){const r=await t({audio:{channelCount:1,echoCancellation:!0,noiseSuppression:!0,autoGainControl:!0},video:!1});if(!this._pressActive||i!==this._pressToken)throw r.getTracks().forEach(p=>p.stop()),new Error("cancelled");this._mediaStream=r,this._sendFill=0;const o=window.AudioContext||window.webkitAudioContext,n=new o({sampleRate:tl});if(this._audioCtx=n,n.state==="suspended"&&await n.resume(),!this._pressActive||i!==this._pressToken)throw new Error("cancelled");const s=n.createMediaStreamSource(r);this._sourceNode=s;const a=n.createGain();a.gain.value=0,this._muteNode=a;const l=new WebSocket(`ws://${this._wsHost()}:${e}`);if(l.binaryType="arraybuffer",this._ws=l,await new Promise((p,f)=>{const b=()=>{_(),p()},v=()=>{_(),f(new Error(`无法连接 ws://${this._wsHost()}:${e}`))},_=()=>{l.removeEventListener("open",b),l.removeEventListener("error",v)};l.addEventListener("open",b),l.addEventListener("error",v)}),!this._pressActive||i!==this._pressToken)throw new Error("cancelled");const c=n.sampleRate;let d=!1;if(n.audioWorklet)try{if(this._workletUrl=URL.createObjectURL(new Blob([ev],{type:"application/javascript"})),await n.audioWorklet.addModule(this._workletUrl),!this._pressActive||i!==this._pressToken)throw new Error("cancelled");const p=new AudioWorkletNode(n,"pcm-capture-processor");this._workletNode=p,p.port.onmessage=f=>{var v;const b=f.data;if(b&&typeof b=="object"&&"type"in b&&b.type==="flushed"){(v=this._flushDone)==null||v.call(this),this._flushDone=null;return}this._handlePcm(b,c)},s.connect(p),p.connect(a),a.connect(n.destination),d=!0}catch(p){if(p instanceof Error&&p.message==="cancelled")throw p}if(!d){if(!this._pressActive||i!==this._pressToken)throw new Error("cancelled");const f=n.createScriptProcessor(2048,1,1);this._scriptNode=f,f.onaudioprocess=b=>{const v=b.inputBuffer.getChannelData(0);this._handlePcm(v,c)},s.connect(f),f.connect(a),a.connect(n.destination)}}_sendRemainder(){if(!this._ws||this._ws.readyState!==WebSocket.OPEN||this._sendFill<=0)return;const e=this._sendAcc.slice(0,this._sendFill);this._sendFill=0,this._ws.send(e.buffer)}async _flushOutgoing(){const e=this._workletNode;e&&await new Promise(t=>{const i=window.setTimeout(()=>{this._flushDone=null,t()},80);this._flushDone=()=>{window.clearTimeout(i),t()};try{e.port.postMessage({type:"flush"})}catch{window.clearTimeout(i),this._flushDone=null,t()}}),this._scriptNode&&await new Promise(t=>window.setTimeout(t,60)),this._sendRemainder()}async _waitSocketDrain(){const e=this._ws;if(!e||e.readyState!==WebSocket.OPEN)return;const t=performance.now();await new Promise(i=>{const r=()=>{if(!this._ws||this._ws.readyState!==WebSocket.OPEN||this._ws.bufferedAmount===0||performance.now()-t>400){i();return}window.setTimeout(r,15)};r()})}_stopTalking(){return this._stopChain?this._stopChain:(this._stopChain=this._finishStop().finally(()=>{this._stopChain=null}),this._stopChain)}async _finishStop(){var e;if(this._pointerId=void 0,this._ready=!1,this._connecting=!1,(this._draining||((e=this._ws)==null?void 0:e.readyState)===WebSocket.OPEN)&&(this._draining=!0,await this._flushOutgoing()),this._draining=!1,this._sendFill=0,this._workletNode){try{this._workletNode.port.onmessage=null,this._workletNode.disconnect()}catch{}this._workletNode=null}if(this._scriptNode){try{this._scriptNode.onaudioprocess=null,this._scriptNode.disconnect()}catch{}this._scriptNode=null}if(this._sourceNode){try{this._sourceNode.disconnect()}catch{}this._sourceNode=null}if(this._muteNode){try{this._muteNode.disconnect()}catch{}this._muteNode=null}if(this._audioCtx){try{await this._audioCtx.close()}catch{}this._audioCtx=null}if(this._workletUrl&&(URL.revokeObjectURL(this._workletUrl),this._workletUrl=null),this._mediaStream&&(this._mediaStream.getTracks().forEach(t=>t.stop()),this._mediaStream=null),await this._waitSocketDrain(),this._ws){try{this._ws.close()}catch{}this._ws=null}}render(){var f;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,i=this._portNumber(),r=String(this._config.name??((f=t==null?void 0:t.attributes)==null?void 0:f.friendly_name)??"对讲"),o=String(this._config.icon??"mdi:microphone"),n=!!this._config.vertical,s=this._ready,a=this._connecting,l=!!(this._error&&!s&&!a),c=!!ma().getUserMedia,d=this._error?this._error:s?"可以说话 · 松开结束":a?"连接中，请稍候…":c?i?`按住说话 · :${i}`:"请绑定 Web Mic /port":"麦克风不可用",p=P({"--tile-color":dr(s,this._config.color)});return h`
      <ha-card
        class=${T({active:s,ready:s,connecting:a,error:l})}
        style=${p}
        tabindex="-1"
        @pointerdown=${this._startTalking}
        @pointerup=${this._onPointerUp}
        @pointercancel=${this._onPointerUp}
        @lostpointercapture=${this._onPointerUp}
        @contextmenu=${this._onContextMenu}
      >
        <ha-tile-container ?vertical=${n} ?fixed-info-height=${n}>
          <ha-tile-icon slot="icon" .icon=${o} .active=${s} interactive></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${r} .secondary=${d}></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `}};Bt.styles=[me,be,w`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        touch-action: none;
        user-select: none;
        -webkit-user-select: none;
        -webkit-touch-callout: none;
      }
      ha-card {
        height: 100%;
        min-height: 0;
        cursor: pointer;
        overflow: hidden;
        -webkit-tap-highlight-color: transparent;
        transition:
          box-shadow 160ms ease,
          border-color 160ms ease,
          opacity 120ms ease;
      }
      /* Connecting: pressed but not live yet — muted, no "ready" color. */
      ha-card.connecting {
        opacity: 0.85;
        box-shadow:
          var(--ha-card-box-shadow),
          0 0 0 2px color-mix(in srgb, var(--secondary-text-color) 35%, transparent);
      }
      /* Ready: WS connected — clear go-ahead to speak. */
      ha-card.ready {
        --tile-color: var(--primary-color);
        opacity: 1;
        box-shadow:
          var(--ha-card-box-shadow),
          0 0 0 2px color-mix(in srgb, var(--tile-color, var(--primary-color)) 60%, transparent),
          0 0 14px color-mix(in srgb, var(--tile-color, var(--primary-color)) 25%, transparent);
      }
      ha-card.error {
        border-color: var(--error-color, #f44336);
      }
    `];ao([u({attribute:!1})],Bt.prototype,"flow",2);ao([g()],Bt.prototype,"_config",2);ao([g()],Bt.prototype,"_ready",2);ao([g()],Bt.prototype,"_connecting",2);ao([g()],Bt.prototype,"_error",2);Bt=ao([x("hui-mic-card")],Bt);var iv=Object.defineProperty,rv=Object.getOwnPropertyDescriptor,sc=(e,t,i,r)=>{for(var o=r>1?void 0:r?rv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&iv(t,i,o),o};const ov=new Set(["on","true","1","ok","normal","connected","open","playing","ready","正常"]),nv=new Set(["off","false","0","error","fail","failed","disconnected","closed","unavailable","异常","错误","断开"]);function pd(e){return Array.isArray(e)?e.map(t=>{if(!t||typeof t!="object")return null;const i=t,r=String(i.entity??"").trim();if(!r)return null;const o=i.name!=null?String(i.name):void 0,n=i.icon!=null?String(i.icon):void 0;return{entity:r,...o?{name:o}:{},...n?{icon:n}:{}}}).filter(t=>t!==null):[]}function sv(e){if(!e||e.state===null||e.state===void 0||e.state==="")return"unknown";const t=e.state;if(typeof t=="boolean")return t?"ok":"error";if(typeof t=="number")return t>0?"ok":"error";if(Array.isArray(t))return t.length>0?"ok":"error";const i=String(t).toLowerCase().trim();return ov.has(i)?"ok":nv.has(i)?"error":$e(e)?"ok":"error"}function av(e){return e==="ok"?"正常":e==="error"?"异常":"—"}let Yr=class extends y{constructor(){super(...arguments),this._unsubs=[],this._subscribedKey=""}static getStubConfig(){return{type:"status",name:"Flow 连接",icon:"mdi:lan-connect",color:"green",entities:[{entity:"/demo/status_ws",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_tcp",name:"tcp连接",icon:"mdi:lan-connect"},{entity:"/demo/status_udp",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_osc",name:"connected",icon:"mdi:lan-connect"}]}}setConfig(e){this._config=e,this._syncSubscriptions(),this.requestUpdate()}connectedCallback(){super.connectedCallback(),this._syncSubscriptions()}disconnectedCallback(){this._clearSubscriptions(),super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("_config"))&&this._syncSubscriptions()}_clearSubscriptions(){this._unsubs.forEach(e=>e()),this._unsubs=[],this._subscribedKey=""}_syncSubscriptions(){var r;const e=this.flow,t=pd((r=this._config)==null?void 0:r.entities),i=`${e?"1":"0"}:${t.map(o=>o.entity).join("\0")}`;i===this._subscribedKey&&this._unsubs.length>0||(this._clearSubscriptions(),e&&(this._subscribedKey=i,t.forEach(o=>{this._unsubs.push(e.subscribeEntity(o.entity,()=>{this.requestUpdate()}))})))}getGridOptions(){var t;const e=!!((t=this._config)!=null&&t.vertical);return{columns:e?3:6,rows:"auto",min_columns:e?3:6,min_rows:2}}render(){var _;if(!this._config)return m;(_=this.flow)==null||_.statesRevision;const e=pd(this._config.entities),t=String(this._config.name??"状态"),i=String(this._config.icon??"mdi:lan-connect"),r=!!this._config.vertical,o=Number(this._config.list_columns)===2?2:1,n=El(this._config.color)??"var(--green-color, #4caf50)",s=e.map(k=>{var C;return sv((C=this.flow)==null?void 0:C.states[k.entity])}),a=s.filter(k=>k==="ok").length,l=e.length,c=l>0&&a===l,d=s.some(k=>k==="error"),p=l===0?"unknown":c?"ok":d?"error":"unknown",f=l===0?"无绑定":`${a}/${l} ${p==="ok"?"正常":p==="error"?"异常":"未知"}`,b=p==="error"?"var(--red-color, #f44336)":p==="unknown"?"var(--state-inactive-color, #9e9e9e)":n,v=P({"--status-ok":n,"--status-accent":b,"--tile-color":b});return h`
      <ha-card
        class=${T({vertical:r,ok:p==="ok",error:p==="error",unknown:p==="unknown"})}
        style=${v}
      >
        <ha-tile-container ?vertical=${r} expand-features>
          <ha-tile-icon slot="icon" .icon=${i}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${t}
            .secondary=${f}
          ></ha-tile-info>
          <div slot="features" class=${T({list:!0,"cols-2":o===2})}>
            ${e.length===0?h`<p class="empty">添加地址后显示各项状态</p>`:e.map((k,C)=>{var R,Q,de,G,Ie;const O=(R=this.flow)==null?void 0:R.states[k.entity],I=s[C]??"unknown",V=((Q=k.name)==null?void 0:Q.trim())||String(((de=O==null?void 0:O.attributes)==null?void 0:de.friendly_name)??"")||k.entity,K=((G=k.icon)==null?void 0:G.trim())||String(((Ie=O==null?void 0:O.attributes)==null?void 0:Ie.icon)??"")||i;return h`
                    <div
                      class=${T({row:!0,ok:I==="ok",error:I==="error",unknown:I==="unknown"})}
                    >
                      <ha-icon class="row-icon" .icon=${K}></ha-icon>
                      <span class="row-name">${V}</span>
                      <span class="row-status">
                        <span class="dot"></span>
                        <span class="status-text">${av(I)}</span>
                      </span>
                    </div>
                  `})}
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Yr.styles=w`
    :host {
      display: block;
      height: 100%;
      --status-ok: var(--green-color, #4caf50);
      --status-error: var(--red-color, #f44336);
      --status-unknown: var(--state-inactive-color, #9e9e9e);
      --status-accent: var(--status-ok);
    }

    ha-card {
      height: 100%;
      --tile-color: var(--status-accent);
    }

    ha-tile-icon {
      --tile-icon-color: var(--status-accent);
    }

    ha-card.ok ha-tile-info {
      --ha-tile-info-secondary-color: var(--status-ok);
    }

    ha-card.error ha-tile-info {
      --ha-tile-info-secondary-color: var(--status-error);
    }

    ha-card.unknown ha-tile-info {
      --ha-tile-info-secondary-color: var(--status-unknown);
    }

    ha-tile-info {
      --ha-tile-info-secondary-color: var(--secondary-text-color);
    }

    .list {
      display: grid;
      grid-template-columns: 1fr;
      gap: 8px;
      width: 100%;
      box-sizing: border-box;
      min-height: 0;
    }

    .list.cols-2 {
      grid-template-columns: 1fr 1fr;
    }

    .empty {
      margin: 0;
      font-size: 12px;
      color: var(--secondary-text-color);
    }

    .row {
      display: flex;
      align-items: center;
      gap: 10px;
      min-height: 40px;
      padding: 8px 12px;
      border-radius: 10px;
      background: var(--ha-color-form-background, #f5f5f5);
      box-sizing: border-box;
      --row-status-color: var(--status-unknown);
    }

    .row.ok {
      --row-status-color: var(--status-ok);
    }

    .row.error {
      --row-status-color: var(--status-error);
    }

    .row-icon {
      flex-shrink: 0;
      --mdc-icon-size: 18px;
      width: 18px;
      height: 18px;
      color: var(--row-status-color);
    }

    .row-name {
      flex: 1;
      min-width: 0;
      font-size: var(--ha-font-size-s, 12px);
      color: var(--primary-text-color);
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }

    .row-status {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      flex-shrink: 0;
    }

    .dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--row-status-color);
    }

    .status-text {
      font-size: var(--ha-font-size-s, 12px);
      color: var(--secondary-text-color);
    }
  `;sc([u({attribute:!1})],Yr.prototype,"flow",2);sc([g()],Yr.prototype,"_config",2);Yr=sc([x("hui-status-card")],Yr);var lv=Object.defineProperty,cv=Object.getOwnPropertyDescriptor,Ys=(e,t,i,r)=>{for(var o=r>1?void 0:r?cv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&lv(t,i,o),o};const ud=[{x:10,y:88},{x:28,y:18},{x:72,y:18},{x:90,y:88}],ba="primary",wp="accent",fd="mdi:map-marker",va=26,_a=1,dv="http://www.w3.org/2000/svg",gd=0,md=1,hv=[bi({type:"entity",entity:"/demo/path_progress",name:"轨迹点 1",icon:"mdi:robot-industrial",color:wp,show_icon:!0,show_name:!1,show_state:!1})];function bd(e){return Math.min(100,Math.max(0,e))}function is(e){return{x:Number(bd(Number(e.x)).toFixed(2)),y:Number(bd(Number(e.y)).toFixed(2))}}function ya(e){if(e.length===0)return"";if(e.length===1){const n=is(e[0]);return`M ${n.x} ${n.y}`}if(e.length===2){const[n,s]=e.map(is);return`M ${n.x} ${n.y} L ${s.x} ${s.y}`}const t=e.map(is);let i=`M ${t[0].x} ${t[0].y}`;for(let n=1;n<t.length-1;n+=1){const s=t[n],a=t[n+1],l=Number(((s.x+a.x)/2).toFixed(2)),c=Number(((s.y+a.y)/2).toFixed(2));i+=` Q ${s.x} ${s.y} ${l} ${c}`}const r=t[t.length-2],o=t[t.length-1];return i+=` Q ${r.x} ${r.y} ${o.x} ${o.y}`,i}function vd(e){return bi(Ge(e))}function pv(e,t){if(e.trim())try{const i=document.createElementNS(dv,"path");i.setAttribute("d",e);const r=i.getTotalLength();if(!Number.isFinite(r)||r<=0)return;const o=i.getPointAtLength(r*t);return{x:o.x,y:o.y}}catch{return}}let ir=class extends y{constructor(){super(...arguments),this._progressRevision=0,this._entityUnsubs=[]}static getStubConfig(){return{type:"path-badge",image:Qo,points:ud.map(e=>({...e})),path:ya(ud),badges:hv.map(e=>({...e})),track_color:ba,badge_size:va,track_width:_a}}setConfig(e){const t=Array.isArray(e.points)?e.points:[],i=this._badgesFromConfig(e),r=t.length>0?ya(t):String(e.path??"");if(!r.trim())throw new Error("Path badge card requires a path");if(i.length===0)throw new Error("Path badge card requires at least one badge");this._config={badge_size:va,track_width:_a,track_color:ba,progress_min:gd,progress_max:md,...e,badges:i,...t.length>0?{points:t.map(is)}:{},path:r},this._syncEntitySubscription(),this.requestUpdate()}getGridOptions(){return{columns:12,rows:"auto",min_columns:6,min_rows:3}}connectedCallback(){super.connectedCallback(),this._syncEntitySubscription()}disconnectedCallback(){this._clearEntitySubscription(),super.disconnectedCallback()}updated(e){super.updated(e),e.has("flow")&&this._syncEntitySubscription()}render(){var s;if(!this._config)return m;(s=this.flow)==null||s.statesRevision;const e=String(this._config.title??""),t=String(this._config.image??""),i=this._activeBadgesFromConfig(this._config),r=this._pathData(),o=Nt(String(this._config.track_color??ba)),n=String(this._config.caption??"").trim();return h`
      <ha-card>
        ${e?h`<h1 class="card-header">${e}</h1>`:m}
        <div class="root">
          ${t?h`<img class="background" alt="" src=${t} draggable="false" />`:h`<div class="placeholder">添加底图后，可让徽章沿轨迹运动</div>`}

          <svg class="overlay" viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
            <path
              id="motion-path"
              d=${r}
              fill="none"
              stroke=${o}
              stroke-width=${String(this._config.track_width??_a)}
              stroke-linecap="round"
              stroke-linejoin="round"
            ></path>
          </svg>

          ${i.map(a=>{var p,f;const l=(p=this.flow)==null?void 0:p.states[a.entity],c=String(a.name??((f=l==null?void 0:l.attributes)==null?void 0:f.friendly_name)??a.entity??"轨迹徽章"),d=pv(r,this._progressForBadge(a));return d?h`
                <div
                  title=${c}
                  style=${[`left:${d.x}%`,`top:${d.y}%`].join(";")}
                  class=${oi(a)?"badge interactive":"badge"}
                >
                  ${this._renderBadgeContent(a,l)}
                </div>
              `:m})}

          ${n?h`<div class="caption">${n}</div>`:m}
        </div>
      </ha-card>
    `}_badgesFromConfig(e){const t=Array.isArray(e.badges)?e.badges.map(vd):[];if(t.length>0)return t;const i=String(e.entity??"").trim();return i?[vd({entity:i,type:"entity",name:String(e.name??"").trim()||void 0,icon:String(e.icon??"").trim()||void 0,color:String(e.badge_color??"").trim()||wp,show_icon:!0,show_name:!1,attribute:String(e.attribute??"").trim()||void 0,show_state:!!e.show_state})]:[]}_activeBadgesFromConfig(e){return this._badgesFromConfig(e).filter(t=>t.entity)}_pathData(){if(!this._config)return"";const e=Array.isArray(this._config.points)?this._config.points:[];return e.length>0?ya(e):String(this._config.path??"")}_progressForBadge(e){var l,c;if(!this.flow)return 0;const t=this.flow.states[e.entity];if(!t)return 0;const i=String(e.attribute??"").trim(),r=i&&i in t.attributes?t.attributes[i]:t.state,o=Number(r);if(!Number.isFinite(o))return 0;const n=Number(((l=this._config)==null?void 0:l.progress_min)??gd),s=Number(((c=this._config)==null?void 0:c.progress_max)??md);if(!Number.isFinite(n)||!Number.isFinite(s))return Math.min(1,Math.max(0,o));if(s===n)return o>=s?1:0;const a=(o-n)/(s-n);return Math.min(1,Math.max(0,a))}_syncEntitySubscription(){if(this._clearEntitySubscription(),!this.flow||!this._config)return;const e=[...new Set(this._activeBadgesFromConfig(this._config).map(t=>t.entity))];this._entityUnsubs=e.map(t=>this.flow.subscribeEntity(t,()=>{this._progressRevision+=1}))}_clearEntitySubscription(){this._entityUnsubs.forEach(e=>e()),this._entityUnsubs=[]}_displayForBadge(e,t){const i=String(t.attribute??"").trim();if(!i||!e){const s=up(this.flow,t.entity,t.unit);return{value:s.value,unit:s.unit}}const r=i&&i in e.attributes?e.attributes[i]:e.state,o=String(t.unit??e.attributes.unit_of_measurement??"").trim();if(typeof r=="boolean")return{value:r?"开":"关",unit:""};if(r==null||r==="")return{value:"—",unit:""};const n=Number(r);return Number.isFinite(n)?{value:Number.isInteger(n)?String(n):n.toFixed(1).replace(/\.0$/,""),unit:o}:{value:String(r),unit:o}}_stateBadgeColor(e,t){const i=t.color?String(t.color).trim():"";return $e(e)?!i||i==="none"?"":i==="state"?"var(--state-icon-color)":i:"var(--state-inactive-color)"}_cardBadgeSizeFallback(){var e;return Number(((e=this._config)==null?void 0:e.badge_size)??va)}_renderBadgeContent(e,t){var n,s;const i=fn(e,this._cardBadgeSizeFallback());if(oi(e))return h`
        <hui-picture-action-element
          .flow=${this.flow}
          .config=${this._buttonConfig(e,i)}
          .preview=${!1}
          .statesRevision=${((n=this.flow)==null?void 0:n.statesRevision)??0}
        ></hui-picture-action-element>
      `;const r=this._displayForBadge(t,e),o=String(e.icon??((s=t==null?void 0:t.attributes)==null?void 0:s.icon)??fd);return h`
      <ha-state-badge
        .value=${r.value}
        .unit=${r.unit}
        .icon=${o}
        .color=${this._stateBadgeColor(t,e)}
        .showIcon=${Sl(e)}
        .showValue=${Rt(e)}
        .size=${i}
        .showBackground=${di(e)}
      ></ha-state-badge>
    `}_buttonConfig(e,t){return bi({...Ge(e),type:"button",text:e.text??e.name,icon:e.icon??fd,show_name:Rt(e),show_background:di(e),size:t})}};ir.styles=w`
    :host {
      display: block;
      height: 100%;
    }

    ha-card {
      height: 100%;
      overflow: hidden;
    }

    .card-header {
      margin: 0;
      padding: 12px 16px 0;
      font-size: 16px;
      font-weight: 600;
    }

    .root {
      position: relative;
      overflow: hidden;
      background: var(--secondary-background-color, #f4f6f8);
    }

    .background {
      position: relative;
      display: block;
      width: 100%;
      height: auto;
      user-select: none;
      pointer-events: none;
    }

    .overlay {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
      z-index: 1;
    }

    .placeholder {
      min-height: 220px;
      height: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--secondary-text-color);
      font-size: 13px;
      background: repeating-linear-gradient(
        45deg,
        var(--secondary-background-color, #f3f4f6),
        var(--secondary-background-color, #f3f4f6) 12px,
        transparent 12px,
        transparent 24px
      );
    }

    .badge {
      position: absolute;
      z-index: 2;
      transform: translate(-50%, -50%);
      pointer-events: none;
      transform-origin: center center;
    }

    .badge.interactive {
      pointer-events: auto;
    }

    .caption {
      position: absolute;
      right: 12px;
      bottom: 12px;
      z-index: 2;
      padding: 8px 10px;
      border-radius: 10px;
      background: color-mix(in srgb, var(--card-background-color, #fff) 84%, transparent);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.12);
      backdrop-filter: blur(4px);
      color: var(--primary-text-color);
      font-size: 12px;
      line-height: 1.4;
    }
  `;Ys([u({attribute:!1})],ir.prototype,"flow",2);Ys([g()],ir.prototype,"_config",2);Ys([g()],ir.prototype,"_progressRevision",2);ir=Ys([x("hui-path-badge-card")],ir);function yo(e,t,i,r,o,n){const s={type:e,name:t,description:i,icon:r,category:"sensor",lazy:!0,loader:async()=>{const a=await o();s.constructor=a[n]}};B(s)}function uv(){B({type:"heading",name:"标题",description:"分区标题行（含状态徽章与控制徽章）",icon:"mdi:format-title",category:"container",constructor:Ir}),B({type:"tile",name:"磁贴",description:"开关切换磁贴",icon:"mdi:view-grid",category:"tile",constructor:Br}),B({type:"sensor",name:"数值",description:"显示地址数值",icon:"mdi:gauge",category:"sensor",constructor:Vr}),B({type:"label",name:"标签",description:"只读显示字符串（设备名、素材名、错误信息等）",icon:"mdi:label-outline",category:"sensor",constructor:Nr}),B({type:"status",name:"状态",description:"多地址状态列表（汇总正常/异常）",icon:"mdi:lan-connect",category:"sensor",constructor:Yr}),B({type:"clock",name:"时钟",description:"数字/模拟时钟",icon:"mdi:clock-outline",category:"container",constructor:Ur}),B({type:"climate",name:"步进",description:"步进磁贴（目标值 +/-）",icon:"mdi:home-thermometer",category:"control",constructor:jr}),B({type:"cover",name:"单选",description:"单选磁贴（位置分段）",icon:"mdi:window-shutter",category:"control",constructor:Gr}),B({type:"trigger",name:"按钮",description:"触发器（收到 true 时短暂亮起）",icon:"mdi:gesture-tap-button",category:"control",constructor:Zi}),B({type:"switch",name:"开关",description:"开关磁贴（Flood light 风格）",icon:"mdi:toggle-switch",category:"control",constructor:Fr}),B({type:"slider",name:"滑块",description:"亮度滑块（Spotlights 风格）",icon:"mdi:tune-vertical",category:"control",constructor:Rr}),B({type:"gain",name:"增益",description:"增益调节（dB 步进，点击静音键降至最小值并锁定）",icon:"mdi:volume-high",category:"control",constructor:Qi}),B({type:"rgba",name:"RGBA",description:"RGBA 颜色选择（绑定 [r,g,b,a] 0–1 地址）",icon:"mdi:palette",category:"control",constructor:Jo}),B({type:"hsv",name:"HSV",description:"HSV 颜色选择（绑定 [h,s,v] 0–1 地址）",icon:"mdi:palette-swatch",category:"control",constructor:Qa}),B({type:"color",name:"颜色（旧）",description:"已更名为 RGBA，保留兼容",icon:"mdi:palette",category:"control",constructor:Ka}),B({type:"multi-fader",name:"多维推杆",description:"多个推杆，输出 N 维数组到同一地址",icon:"mdi:tune-vertical-variant",category:"control",constructor:er}),B({type:"xy-pad",name:"XY 触控板",description:"二维触控板（绑定 [x,y] 或分别绑定 X/Y 地址）",icon:"mdi:axis-arrow",category:"control",constructor:tr}),B({type:"media",name:"媒体",description:"播放器启停控制（播放 / 停止）",icon:"mdi:cast-audio",category:"control",constructor:Wr}),B({type:"mic",name:"对讲麦克风",description:"按住说话，实时 PCM 推送到 Web Mic 节点",icon:"mdi:microphone",category:"control",constructor:Bt}),B({type:"markdown",name:"Markdown",description:"文本说明卡片",icon:"mdi:text",category:"container",constructor:zr}),B({type:"link",name:"超链接",description:"点击跳转到外部或内部链接",icon:"mdi:link-variant",category:"container",constructor:Xr}),B({type:"picture-elements",name:"图片元素",description:"在底图上叠加状态徽章与控制徽章",icon:"mdi:floor-plan",category:"container",constructor:mt}),B({type:"path-badge",name:"轨迹元素",description:"在底图上绘制轨迹，并让多个徽章按各自的 0-1 值沿轨迹运动",icon:"mdi:map-marker-path",category:"container",constructor:ir}),yo("line3d","3D 折线","实时三维折线图（订阅 OSC 地址推送的点坐标）","mdi:chart-timeline-variant",()=>Sr(()=>import("./hui-line3d-card-BuCWIuVa.js"),__vite__mapDeps([0,1,2]),import.meta.url),"HuiLine3dCard"),yo("scatter3d","3D 散点","实时三维散点图（订阅 OSC 地址推送的点坐标）","mdi:chart-scatter-plot",()=>Sr(()=>import("./hui-scatter3d-card-BGC5RYAw.js"),__vite__mapDeps([3,1,2]),import.meta.url),"HuiScatter3dCard"),yo("line2d","2D 折线","实时二维折线图（订阅 OSC 地址推送的 [x,y] 坐标）","mdi:chart-line",()=>Sr(()=>import("./hui-line2d-card-CaMj3KM_.js"),__vite__mapDeps([4,5,2]),import.meta.url),"HuiLine2dCard"),yo("scatter2d","2D 散点","实时二维散点图（订阅 OSC 地址推送的 [x,y] 坐标）","mdi:chart-scatter-plot-hexbin",()=>Sr(()=>import("./hui-scatter2d-card-BN980qKi.js"),__vite__mapDeps([6,5,2]),import.meta.url),"HuiScatter2dCard"),yo("bar","柱状图","每根柱绑定一个地址，显示该地址的实时数值","mdi:chart-bar",()=>Sr(()=>import("./hui-bar-card-CDc7csu6.js"),__vite__mapDeps([7,5,2]),import.meta.url),"HuiBarCard"),B({type:"error",name:"Error",category:"container",constructor:Go})}uv();let Co=null,So=null;function il(e){return new Promise(t=>{Co=t,document.dispatchEvent(new CustomEvent("flow-show-confirm",{bubbles:!0,composed:!0,detail:e}))})}function fv(e){Co==null||Co(e),Co=null}function gv(e){return new Promise(t=>{So=t,document.dispatchEvent(new CustomEvent("flow-show-delete-card",{bubbles:!0,composed:!0,detail:{cardConfig:e}}))})}function mv(e){So==null||So(e),So=null}function $p(e,t,i){const r=[...e],[o]=r.splice(t,1);return r.splice(i,0,o),r}function bv(e,t){return e.views[t]}function ni(e,t,i){e.sections=e.sections.map((r,o)=>o===t?i:r)}function vv(e,t){e.sections=[...e.sections,t]}function _v(e,t){return{...e,cards:[...e.cards,t]}}function yv(e){if(Array.isArray(e)&&e.length===3){const[t,i,r]=e;if(typeof t=="number"&&typeof i=="number"&&typeof r=="number")return{viewIndex:t,sectionIndex:i,cardIndex:r}}if(e&&typeof e=="object"){const t=e;if(typeof t.viewIndex=="number"&&typeof t.sectionIndex=="number"&&typeof t.cardIndex=="number")return t}}function _d(e,t,i,r,o){const n=e.sections[t],s=e.sections[r];if(!n||!s)return;const a=n.cards[i];if(!a)return;if(t===r){ni(e,t,{...n,cards:$p(n.cards,i,o)});return}const l=n.cards.filter((p,f)=>f!==i),c=[...s.cards],d=Math.max(0,Math.min(o,c.length));c.splice(d,0,a),ni(e,t,{...n,cards:l}),ni(e,r,{...s,cards:c})}function xv(e,t,i){e.sections=$p(e.sections,t,i)}function wv(e,t){const i=e.cards[t];if(!i)return e;const r=structuredClone(i),o=[...e.cards];return o.splice(t+1,0,r),{...e,cards:o}}function $v(e,t){return{...e,cards:e.cards.filter((i,r)=>r!==t)}}function kv(e,t){const i=e.sections[t];if(!i)return;const r=structuredClone(i),o=[...e.sections];o.splice(t+1,0,r),e.sections=o}function Cv(e,t){e.sections=e.sections.filter((i,r)=>r!==t)}function Sv(e,t,i){if(!e.cards[t])return e;const o=e.cards.map((n,s)=>s===t?{...i}:n);return{...e,cards:o}}function Ev(e,t,i){const r=e.sections[t];if(!r)return;const o={...r,...i};i.background===void 0&&"background"in i&&delete o.background,ni(e,t,o)}function yd(e,t){const i=e.views;if(!i.length||t===void 0||t==="")return 0;if(typeof t=="number")return Math.max(0,Math.min(t,i.length-1));const r=Number(t);for(let o=0;o<i.length;o++)if(i[o].path===t||o===r)return o;return 0}function rl(e,t){return e.path||String(t)}function Pv(e,t,i){const r=[...e.views],o=r.length;return r.splice(o,0,t),e.views=r,o}function Av(e,t){return e.views.length<=1?0:(e.views=e.views.filter((i,r)=>r!==t),Math.min(t,e.views.length-1))}function Tv(e,t,i){e.views[t]&&(e.views=e.views.map((o,n)=>n===t?{...o,...i}:o))}const Mv=`欢迎使用 **Flow Dashboard**。

布局采用 Section + Card 结构，仿照 [Home Assistant Sections](https://www.home-assistant.io/dashboards/sections/)。

- 支持 **粗体** / *斜体*
- 支持列表与链接
- 可在编辑器中切换卡片 / 纯文本样式`;function Ov(){return Ch({title:"Flow",views:[{title:"Home",path:"home",icon:"mdi:home",max_columns:4,sections:[{type:"grid",column_span:4,cards:[{type:"heading",heading:"Welcome",icon:"mdi:hand-wave",badges:[{type:"entity",entity:"/demo/temperature",icon:"mdi:thermometer"},{type:"entity",entity:"/demo/humidity",icon:"mdi:water-percent"}]},{type:"markdown",content:Mv}]},{type:"grid",column_span:1,cards:[{type:"clock",clock_style:"analog",clock_size:"small",time_format:"24",grid_options:{columns:6,rows:2,min_columns:3,min_rows:2}},{type:"media",entity:"/demo/media",name:"媒体播放器",icon:"mdi:cast-audio"},{type:"slider",entity:"/demo/spotlights",name:"Spotlights",icon:"mdi:spotlight-beam"},{type:"rgba",entity:"/demo/rgba",name:"RGBA",icon:"mdi:palette",grid_options:{columns:6,rows:5,min_columns:6,min_rows:5}},{type:"hsv",entity:"/demo/hsv",name:"HSV",icon:"mdi:palette-swatch",grid_options:{columns:6,rows:5,min_columns:6,min_rows:5}},{type:"multi-fader",entity:"/demo/faders",name:"多维推杆",icon:"mdi:tune-vertical-variant",count:4,min:0,max:1,step:.01,labels:"0,1,2,3",orientation:"vertical"},{type:"xy-pad",entity:"/demo/xy",name:"XY Pad",icon:"mdi:axis-arrow"},{type:"climate",entity:"/demo/climate",name:"Upstairs",icon:"mdi:home-thermometer",min:0,max:100,step:1,unit:"°C"},{type:"sensor",entity:"/demo/temperature",name:"Temperature",icon:"mdi:thermometer",unit:"°C"},{type:"label",entity:"/demo/string",name:"当前素材",icon:"mdi:filmstrip"},{type:"status",name:"Flow 连接",icon:"mdi:lan-connect",color:"green",entities:[{entity:"/demo/status_ws",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_tcp",name:"tcp连接",icon:"mdi:lan-connect"},{entity:"/demo/status_udp",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_osc",name:"connected",icon:"mdi:lan-connect"}],grid_options:{columns:6,rows:"auto",min_columns:3}}]},{type:"grid",column_span:1,cards:[{type:"switch",entity:"/demo/floor_lamp",name:"Flood light",icon:"mdi:lightbulb"},{type:"cover",entity:"/demo/kitchen_shutter",name:"Kitchen shutter",icon:"mdi:window-shutter",position_count:4},{type:"trigger",entity:"/demo/trigger",name:"Trigger",icon:"mdi:gesture-tap-button"}]},{type:"grid",column_span:1,cards:[{type:"gain",entity:"/demo/gain",name:"输入增益",icon:"mdi:volume-high",min:-60,max:12,step:1}]}]},{title:"设备",path:"devices",icon:"mdi:devices",max_columns:4,sections:[{type:"grid",column_span:2,cards:[{type:"heading",heading:"连接状态",icon:"mdi:lan-connect"},{type:"status",name:"Flow 连接",icon:"mdi:lan-connect",color:"green",list_columns:2,entities:[{entity:"/demo/status_ws",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_tcp",name:"tcp连接",icon:"mdi:lan-connect"},{entity:"/demo/status_udp",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_osc",name:"connected",icon:"mdi:lan-connect"}]},{type:"tile",entity:"/demo/lamp",name:"台灯",icon:"mdi:desk-lamp"},{type:"switch",entity:"/demo/switch",name:"开关",icon:"mdi:toggle-switch"}]}]}]})}const zn=Ov();function Lv(e){return e.version===2?!Array.isArray(e.views)||e.views.length===0?!0:e.views.every(t=>{const i=t.sections??[];return i.length===0?!0:i.every(r=>{var o;return(((o=r.cards)==null?void 0:o.length)??0)===0})}):Array.isArray(e.items)?e.items.length===0:!0}async function Dv(){try{const e=await fetch("/api/layout/load");if(!e.ok)return zn;const t=await e.json();return Lv(t)?zn:t.version===2&&Array.isArray(t.views)?Ch({title:"Flow",views:t.views}):zn}catch{return zn}}async function Iv(e){await fetch("/api/layout/save",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({version:2,...e})})}function kp(e){const t=new Set;return e.views.forEach(i=>{i.sections.forEach(r=>{r.cards.forEach(o=>{o.entity&&t.add(o.entity);const n=o.entity_x;typeof n=="string"&&n&&t.add(n);const s=o.entity_y;typeof s=="string"&&s&&t.add(s);const a=o.badges;a==null||a.forEach(d=>{d.entity&&t.add(d.entity)});const l=o.elements;l==null||l.forEach(d=>{d.entity&&t.add(d.entity)});const c=o.entities;c==null||c.forEach(d=>{d.entity&&t.add(d.entity)})})})}),[...t]}const Cp="flow_layout_edit_enabled",ol="flow-layout-edit-enabled-changed";function Sp(){try{return localStorage.getItem(Cp)==="true"}catch{return!1}}function Hv(e){try{localStorage.setItem(Cp,e?"true":"false")}catch{}document.dispatchEvent(new CustomEvent(ol,{detail:{enabled:e}}))}const Ep="dashboardCardClipboard",Pp="dashboardRecentCardTypes",zv=4,nl="flow-dashboard-card-clipboard-changed";function Vv(){try{const e=sessionStorage.getItem(Ep);if(!e)return;const t=JSON.parse(e);return!t||typeof t!="object"||!t.type?void 0:t}catch{return}}function Nv(e){try{sessionStorage.setItem(Ep,JSON.stringify(e))}catch{}document.dispatchEvent(new CustomEvent(nl))}function Rv(e){return document.addEventListener(nl,e),window.addEventListener("storage",e),()=>{document.removeEventListener(nl,e),window.removeEventListener("storage",e)}}function Ap(){try{const e=localStorage.getItem(Pp);if(!e)return[];const t=JSON.parse(e);return Array.isArray(t)?t.filter(i=>typeof i=="string"&&!!i):[]}catch{return[]}}function Fv(e){if(!e||e==="error"||e==="color")return;const t=[e,...Ap().filter(i=>i!==e)].slice(0,zv);try{localStorage.setItem(Pp,JSON.stringify(t))}catch{}}const Bv=["heading","tile","sensor","status"],xd=4,Uv=75;class jv{constructor(t){this._undoStack=[],this._redoStack=[],this._stackLimit=t.stackLimit??Uv,this._currentConfig=t.currentConfig,this._apply=t.apply,this._onStackChange=t.onStackChange}get canUndo(){return this._undoStack.length>0}get canRedo(){return this._redoStack.length>0}commit(t){var i;this._undoStack.length>=this._stackLimit&&this._undoStack.shift(),this._undoStack.push(t),this._redoStack=[],(i=this._onStackChange)==null||i.call(this)}undo(){var i;if(this._undoStack.length===0)return;this._redoStack.push(this._currentConfig());const t=this._undoStack.pop();this._apply(t),(i=this._onStackChange)==null||i.call(this)}redo(){var i;if(this._redoStack.length===0)return;this._undoStack.push(this._currentConfig());const t=this._redoStack.pop();this._apply(t),(i=this._onStackChange)==null||i.call(this)}reset(){var t;this._undoStack=[],this._redoStack=[],(t=this._onStackChange)==null||t.call(this)}}function qv(e,t){const i=window.matchMedia(e),r=o=>t(o.matches);return i.addEventListener("change",r),t(i.matches),()=>i.removeEventListener("change",r)}/**
 * @license
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const{I:Gv}=rf,wd=e=>e,$d=()=>document.createComment(""),xo=(e,t,i)=>{var n;const r=e._$AA.parentNode,o=t===void 0?e._$AB:t._$AA;if(i===void 0){const s=r.insertBefore($d(),o),a=r.insertBefore($d(),o);i=new Gv(s,a,e,e.options)}else{const s=i._$AB.nextSibling,a=i._$AM,l=a!==e;if(l){let c;(n=i._$AQ)==null||n.call(i,e),i._$AM=e,i._$AP!==void 0&&(c=e._$AU)!==a._$AU&&i._$AP(c)}if(s!==o||l){let c=i._$AA;for(;c!==s;){const d=wd(c).nextSibling;wd(r).insertBefore(c,o),c=d}}}return i},Ti=(e,t,i=e)=>(e._$AI(t,i),e),Xv={},Wv=(e,t=Xv)=>e._$AH=t,Yv=e=>e._$AH,xa=e=>{e._$AR(),e._$AA.remove()};/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const kd=(e,t,i)=>{const r=new Map;for(let o=t;o<=i;o++)r.set(e[o],o);return r},Tp=$l(class extends kl{constructor(e){if(super(e),e.type!==wl.CHILD)throw Error("repeat() can only be used in text expressions")}dt(e,t,i){let r;i===void 0?i=t:t!==void 0&&(r=t);const o=[],n=[];let s=0;for(const a of e)o[s]=r?r(a,s):s,n[s]=i(a,s),s++;return{values:n,keys:o}}render(e,t,i){return this.dt(e,t,i).values}update(e,[t,i,r]){const o=Yv(e),{values:n,keys:s}=this.dt(t,i,r);if(!Array.isArray(o))return this.ut=s,n;const a=this.ut??(this.ut=[]),l=[];let c,d,p=0,f=o.length-1,b=0,v=n.length-1;for(;p<=f&&b<=v;)if(o[p]===null)p++;else if(o[f]===null)f--;else if(a[p]===s[b])l[b]=Ti(o[p],n[b]),p++,b++;else if(a[f]===s[v])l[v]=Ti(o[f],n[v]),f--,v--;else if(a[p]===s[v])l[v]=Ti(o[p],n[v]),xo(e,l[v+1],o[p]),p++,v--;else if(a[f]===s[b])l[b]=Ti(o[f],n[b]),xo(e,o[p],o[f]),f--,b++;else if(c===void 0&&(c=kd(s,b,v),d=kd(a,p,f)),c.has(a[p]))if(c.has(a[f])){const _=d.get(s[b]),k=_!==void 0?o[_]:null;if(k===null){const C=xo(e,o[p]);Ti(C,n[b]),l[b]=C}else l[b]=Ti(k,n[b]),xo(e,o[p],k),o[_]=null;b++}else xa(o[f]),f--;else xa(o[p]),p++;for(;b<=v;){const _=xo(e,l[v+1]);Ti(_,n[b]),l[b++]=_}for(;p<=f;){const _=o[p++];_!==null&&xa(_)}return this.ut=s,Wv(e,l),Vt}}),Mp=4;function Vn(e){return parseInt(e.replace("px","").trim(),10)||0}function Zv(e){return e.reduce((t,i)=>t+(i.column_span??1),0)}function Kv(e,t={}){const i=t.minColumnWidth??320,r=t.columnGap??32,o=t.horizontalPadding??0,n=t.configMaxColumns??Mp,s=Math.floor((e-o+r)/(i+r));return Math.max(1,Math.min(n,s))}function Qv(e){const{sectionColumnSpanSum:t,maxColumns:i,editMode:r=!1,extraSlots:o=0}=e,n=t+(r?1:0)+o;return Math.max(Math.min(i,n),1)}function Jv(e,t){return Math.min(e??1,t)}/**!
 * Sortable 1.15.7
 * @author	RubaXa   <trash@rubaxa.org>
 * @author	owenm    <owen23355@gmail.com>
 * @license MIT
 */function e_(e,t,i){return(t=o_(t))in e?Object.defineProperty(e,t,{value:i,enumerable:!0,configurable:!0,writable:!0}):e[t]=i,e}function Ut(){return Ut=Object.assign?Object.assign.bind():function(e){for(var t=1;t<arguments.length;t++){var i=arguments[t];for(var r in i)({}).hasOwnProperty.call(i,r)&&(e[r]=i[r])}return e},Ut.apply(null,arguments)}function Cd(e,t){var i=Object.keys(e);if(Object.getOwnPropertySymbols){var r=Object.getOwnPropertySymbols(e);t&&(r=r.filter(function(o){return Object.getOwnPropertyDescriptor(e,o).enumerable})),i.push.apply(i,r)}return i}function vt(e){for(var t=1;t<arguments.length;t++){var i=arguments[t]!=null?arguments[t]:{};t%2?Cd(Object(i),!0).forEach(function(r){e_(e,r,i[r])}):Object.getOwnPropertyDescriptors?Object.defineProperties(e,Object.getOwnPropertyDescriptors(i)):Cd(Object(i)).forEach(function(r){Object.defineProperty(e,r,Object.getOwnPropertyDescriptor(i,r))})}return e}function t_(e,t){if(e==null)return{};var i,r,o=i_(e,t);if(Object.getOwnPropertySymbols){var n=Object.getOwnPropertySymbols(e);for(r=0;r<n.length;r++)i=n[r],t.indexOf(i)===-1&&{}.propertyIsEnumerable.call(e,i)&&(o[i]=e[i])}return o}function i_(e,t){if(e==null)return{};var i={};for(var r in e)if({}.hasOwnProperty.call(e,r)){if(t.indexOf(r)!==-1)continue;i[r]=e[r]}return i}function r_(e,t){if(typeof e!="object"||!e)return e;var i=e[Symbol.toPrimitive];if(i!==void 0){var r=i.call(e,t);if(typeof r!="object")return r;throw new TypeError("@@toPrimitive must return a primitive value.")}return(t==="string"?String:Number)(e)}function o_(e){var t=r_(e,"string");return typeof t=="symbol"?t:t+""}function sl(e){"@babel/helpers - typeof";return sl=typeof Symbol=="function"&&typeof Symbol.iterator=="symbol"?function(t){return typeof t}:function(t){return t&&typeof Symbol=="function"&&t.constructor===Symbol&&t!==Symbol.prototype?"symbol":typeof t},sl(e)}var n_="1.15.7";function Dt(e){if(typeof window<"u"&&window.navigator)return!!navigator.userAgent.match(e)}var Xt=Dt(/(?:Trident.*rv[ :]?11\.|msie|iemobile|Windows Phone)/i),xn=Dt(/Edge/i),Sd=Dt(/firefox/i),Lo=Dt(/safari/i)&&!Dt(/chrome/i)&&!Dt(/android/i),ac=Dt(/iP(ad|od|hone)/i),Op=Dt(/chrome/i)&&Dt(/android/i),Lp={capture:!1,passive:!1};function D(e,t,i){e.addEventListener(t,i,!Xt&&Lp)}function L(e,t,i){e.removeEventListener(t,i,!Xt&&Lp)}function _s(e,t){if(t){if(t[0]===">"&&(t=t.substring(1)),e)try{if(e.matches)return e.matches(t);if(e.msMatchesSelector)return e.msMatchesSelector(t);if(e.webkitMatchesSelector)return e.webkitMatchesSelector(t)}catch{return!1}return!1}}function Dp(e){return e.host&&e!==document&&e.host.nodeType&&e.host!==e?e.host:e.parentNode}function Qe(e,t,i,r){if(e){i=i||document;do{if(t!=null&&(t[0]===">"?e.parentNode===i&&_s(e,t):_s(e,t))||r&&e===i)return e;if(e===i)break}while(e=Dp(e))}return null}var Ed=/\s+/g;function Pe(e,t,i){if(e&&t)if(e.classList)e.classList[i?"add":"remove"](t);else{var r=(" "+e.className+" ").replace(Ed," ").replace(" "+t+" "," ");e.className=(r+(i?" "+t:"")).replace(Ed," ")}}function S(e,t,i){var r=e&&e.style;if(r){if(i===void 0)return document.defaultView&&document.defaultView.getComputedStyle?i=document.defaultView.getComputedStyle(e,""):e.currentStyle&&(i=e.currentStyle),t===void 0?i:i[t];!(t in r)&&t.indexOf("webkit")===-1&&(t="-webkit-"+t),r[t]=i+(typeof i=="string"?"":"px")}}function Mr(e,t){var i="";if(typeof e=="string")i=e;else do{var r=S(e,"transform");r&&r!=="none"&&(i=r+" "+i)}while(!t&&(e=e.parentNode));var o=window.DOMMatrix||window.WebKitCSSMatrix||window.CSSMatrix||window.MSCSSMatrix;return o&&new o(i)}function Ip(e,t,i){if(e){var r=e.getElementsByTagName(t),o=0,n=r.length;if(i)for(;o<n;o++)i(r[o],o);return r}return[]}function pt(){var e=document.scrollingElement;return e||document.documentElement}function te(e,t,i,r,o){if(!(!e.getBoundingClientRect&&e!==window)){var n,s,a,l,c,d,p;if(e!==window&&e.parentNode&&e!==pt()?(n=e.getBoundingClientRect(),s=n.top,a=n.left,l=n.bottom,c=n.right,d=n.height,p=n.width):(s=0,a=0,l=window.innerHeight,c=window.innerWidth,d=window.innerHeight,p=window.innerWidth),(t||i)&&e!==window&&(o=o||e.parentNode,!Xt))do if(o&&o.getBoundingClientRect&&(S(o,"transform")!=="none"||i&&S(o,"position")!=="static")){var f=o.getBoundingClientRect();s-=f.top+parseInt(S(o,"border-top-width")),a-=f.left+parseInt(S(o,"border-left-width")),l=s+n.height,c=a+n.width;break}while(o=o.parentNode);if(r&&e!==window){var b=Mr(o||e),v=b&&b.a,_=b&&b.d;b&&(s/=_,a/=v,p/=v,d/=_,l=s+d,c=a+p)}return{top:s,left:a,bottom:l,right:c,width:p,height:d}}}function Pd(e,t,i){for(var r=si(e,!0),o=te(e)[t];r;){var n=te(r)[i],s=void 0;if(s=o>=n,!s)return r;if(r===pt())break;r=si(r,!1)}return!1}function Zr(e,t,i,r){for(var o=0,n=0,s=e.children;n<s.length;){if(s[n].style.display!=="none"&&s[n]!==E.ghost&&(r||s[n]!==E.dragged)&&Qe(s[n],i.draggable,e,!1)){if(o===t)return s[n];o++}n++}return null}function lc(e,t){for(var i=e.lastElementChild;i&&(i===E.ghost||S(i,"display")==="none"||t&&!_s(i,t));)i=i.previousElementSibling;return i||null}function Ve(e,t){var i=0;if(!e||!e.parentNode)return-1;for(;e=e.previousElementSibling;)e.nodeName.toUpperCase()!=="TEMPLATE"&&e!==E.clone&&(!t||_s(e,t))&&i++;return i}function Ad(e){var t=0,i=0,r=pt();if(e)do{var o=Mr(e),n=o.a,s=o.d;t+=e.scrollLeft*n,i+=e.scrollTop*s}while(e!==r&&(e=e.parentNode));return[t,i]}function s_(e,t){for(var i in e)if(e.hasOwnProperty(i)){for(var r in t)if(t.hasOwnProperty(r)&&t[r]===e[i][r])return Number(i)}return-1}function si(e,t){if(!e||!e.getBoundingClientRect)return pt();var i=e,r=!1;do if(i.clientWidth<i.scrollWidth||i.clientHeight<i.scrollHeight){var o=S(i);if(i.clientWidth<i.scrollWidth&&(o.overflowX=="auto"||o.overflowX=="scroll")||i.clientHeight<i.scrollHeight&&(o.overflowY=="auto"||o.overflowY=="scroll")){if(!i.getBoundingClientRect||i===document.body)return pt();if(r||t)return i;r=!0}}while(i=i.parentNode);return pt()}function a_(e,t){if(e&&t)for(var i in t)t.hasOwnProperty(i)&&(e[i]=t[i]);return e}function wa(e,t){return Math.round(e.top)===Math.round(t.top)&&Math.round(e.left)===Math.round(t.left)&&Math.round(e.height)===Math.round(t.height)&&Math.round(e.width)===Math.round(t.width)}var Do;function Hp(e,t){return function(){if(!Do){var i=arguments,r=this;i.length===1?e.call(r,i[0]):e.apply(r,i),Do=setTimeout(function(){Do=void 0},t)}}}function l_(){clearTimeout(Do),Do=void 0}function zp(e,t,i){e.scrollLeft+=t,e.scrollTop+=i}function Vp(e){var t=window.Polymer,i=window.jQuery||window.Zepto;return t&&t.dom?t.dom(e).cloneNode(!0):i?i(e).clone(!0)[0]:e.cloneNode(!0)}function Np(e,t,i){var r={};return Array.from(e.children).forEach(function(o){var n,s,a,l;if(!(!Qe(o,t.draggable,e,!1)||o.animated||o===i)){var c=te(o);r.left=Math.min((n=r.left)!==null&&n!==void 0?n:1/0,c.left),r.top=Math.min((s=r.top)!==null&&s!==void 0?s:1/0,c.top),r.right=Math.max((a=r.right)!==null&&a!==void 0?a:-1/0,c.right),r.bottom=Math.max((l=r.bottom)!==null&&l!==void 0?l:-1/0,c.bottom)}}),r.width=r.right-r.left,r.height=r.bottom-r.top,r.x=r.left,r.y=r.top,r}var we="Sortable"+new Date().getTime();function c_(){var e=[],t;return{captureAnimationState:function(){if(e=[],!!this.options.animation){var r=[].slice.call(this.el.children);r.forEach(function(o){if(!(S(o,"display")==="none"||o===E.ghost)){e.push({target:o,rect:te(o)});var n=vt({},e[e.length-1].rect);if(o.thisAnimationDuration){var s=Mr(o,!0);s&&(n.top-=s.f,n.left-=s.e)}o.fromRect=n}})}},addAnimationState:function(r){e.push(r)},removeAnimationState:function(r){e.splice(s_(e,{target:r}),1)},animateAll:function(r){var o=this;if(!this.options.animation){clearTimeout(t),typeof r=="function"&&r();return}var n=!1,s=0;e.forEach(function(a){var l=0,c=a.target,d=c.fromRect,p=te(c),f=c.prevFromRect,b=c.prevToRect,v=a.rect,_=Mr(c,!0);_&&(p.top-=_.f,p.left-=_.e),c.toRect=p,c.thisAnimationDuration&&wa(f,p)&&!wa(d,p)&&(v.top-p.top)/(v.left-p.left)===(d.top-p.top)/(d.left-p.left)&&(l=h_(v,f,b,o.options)),wa(p,d)||(c.prevFromRect=d,c.prevToRect=p,l||(l=o.options.animation),o.animate(c,v,p,l)),l&&(n=!0,s=Math.max(s,l),clearTimeout(c.animationResetTimer),c.animationResetTimer=setTimeout(function(){c.animationTime=0,c.prevFromRect=null,c.fromRect=null,c.prevToRect=null,c.thisAnimationDuration=null},l),c.thisAnimationDuration=l)}),clearTimeout(t),n?t=setTimeout(function(){typeof r=="function"&&r()},s):typeof r=="function"&&r(),e=[]},animate:function(r,o,n,s){if(s){S(r,"transition",""),S(r,"transform","");var a=Mr(this.el),l=a&&a.a,c=a&&a.d,d=(o.left-n.left)/(l||1),p=(o.top-n.top)/(c||1);r.animatingX=!!d,r.animatingY=!!p,S(r,"transform","translate3d("+d+"px,"+p+"px,0)"),this.forRepaintDummy=d_(r),S(r,"transition","transform "+s+"ms"+(this.options.easing?" "+this.options.easing:"")),S(r,"transform","translate3d(0,0,0)"),typeof r.animated=="number"&&clearTimeout(r.animated),r.animated=setTimeout(function(){S(r,"transition",""),S(r,"transform",""),r.animated=!1,r.animatingX=!1,r.animatingY=!1},s)}}}}function d_(e){return e.offsetWidth}function h_(e,t,i,r){return Math.sqrt(Math.pow(t.top-e.top,2)+Math.pow(t.left-e.left,2))/Math.sqrt(Math.pow(t.top-i.top,2)+Math.pow(t.left-i.left,2))*r.animation}var xr=[],$a={initializeByDefault:!0},wn={mount:function(t){for(var i in $a)$a.hasOwnProperty(i)&&!(i in t)&&(t[i]=$a[i]);xr.forEach(function(r){if(r.pluginName===t.pluginName)throw"Sortable: Cannot mount plugin ".concat(t.pluginName," more than once")}),xr.push(t)},pluginEvent:function(t,i,r){var o=this;this.eventCanceled=!1,r.cancel=function(){o.eventCanceled=!0};var n=t+"Global";xr.forEach(function(s){i[s.pluginName]&&(i[s.pluginName][n]&&i[s.pluginName][n](vt({sortable:i},r)),i.options[s.pluginName]&&i[s.pluginName][t]&&i[s.pluginName][t](vt({sortable:i},r)))})},initializePlugins:function(t,i,r,o){xr.forEach(function(a){var l=a.pluginName;if(!(!t.options[l]&&!a.initializeByDefault)){var c=new a(t,i,t.options);c.sortable=t,c.options=t.options,t[l]=c,Ut(r,c.defaults)}});for(var n in t.options)if(t.options.hasOwnProperty(n)){var s=this.modifyOption(t,n,t.options[n]);typeof s<"u"&&(t.options[n]=s)}},getEventProperties:function(t,i){var r={};return xr.forEach(function(o){typeof o.eventProperties=="function"&&Ut(r,o.eventProperties.call(i[o.pluginName],t))}),r},modifyOption:function(t,i,r){var o;return xr.forEach(function(n){t[n.pluginName]&&n.optionListeners&&typeof n.optionListeners[i]=="function"&&(o=n.optionListeners[i].call(t[n.pluginName],r))}),o}};function p_(e){var t=e.sortable,i=e.rootEl,r=e.name,o=e.targetEl,n=e.cloneEl,s=e.toEl,a=e.fromEl,l=e.oldIndex,c=e.newIndex,d=e.oldDraggableIndex,p=e.newDraggableIndex,f=e.originalEvent,b=e.putSortable,v=e.extraEventProperties;if(t=t||i&&i[we],!!t){var _,k=t.options,C="on"+r.charAt(0).toUpperCase()+r.substr(1);window.CustomEvent&&!Xt&&!xn?_=new CustomEvent(r,{bubbles:!0,cancelable:!0}):(_=document.createEvent("Event"),_.initEvent(r,!0,!0)),_.to=s||i,_.from=a||i,_.item=o||i,_.clone=n,_.oldIndex=l,_.newIndex=c,_.oldDraggableIndex=d,_.newDraggableIndex=p,_.originalEvent=f,_.pullMode=b?b.lastPutMode:void 0;var O=vt(vt({},v),wn.getEventProperties(r,t));for(var I in O)_[I]=O[I];i&&i.dispatchEvent(_),k[C]&&k[C].call(t,_)}}var u_=["evt"],ye=function(t,i){var r=arguments.length>2&&arguments[2]!==void 0?arguments[2]:{},o=r.evt,n=t_(r,u_);wn.pluginEvent.bind(E)(t,i,vt({dragEl:$,parentEl:Z,ghostEl:A,rootEl:U,nextEl:Li,lastDownEl:rs,cloneEl:X,cloneHidden:ri,dragStarted:Eo,putSortable:se,activeSortable:E.active,originalEvent:o,oldIndex:Pr,oldDraggableIndex:Io,newIndex:Ae,newDraggableIndex:ti,hideGhostForTarget:Up,unhideGhostForTarget:jp,cloneNowHidden:function(){ri=!0},cloneNowShown:function(){ri=!1},dispatchSortableEvent:function(a){he({sortable:i,name:a,originalEvent:o})}},n))};function he(e){p_(vt({putSortable:se,cloneEl:X,targetEl:$,rootEl:U,oldIndex:Pr,oldDraggableIndex:Io,newIndex:Ae,newDraggableIndex:ti},e))}var $,Z,A,U,Li,rs,X,ri,Pr,Ae,Io,ti,Nn,se,Er=!1,ys=!1,xs=[],Mi,Ze,ka,Ca,Td,Md,Eo,wr,Ho,zo=!1,Rn=!1,os,ae,Sa=[],al=!1,ws=[],Zs=typeof document<"u",Fn=ac,Od=xn||Xt?"cssFloat":"float",f_=Zs&&!Op&&!ac&&"draggable"in document.createElement("div"),Rp=function(){if(Zs){if(Xt)return!1;var e=document.createElement("x");return e.style.cssText="pointer-events:auto",e.style.pointerEvents==="auto"}}(),Fp=function(t,i){var r=S(t),o=parseInt(r.width)-parseInt(r.paddingLeft)-parseInt(r.paddingRight)-parseInt(r.borderLeftWidth)-parseInt(r.borderRightWidth),n=Zr(t,0,i),s=Zr(t,1,i),a=n&&S(n),l=s&&S(s),c=a&&parseInt(a.marginLeft)+parseInt(a.marginRight)+te(n).width,d=l&&parseInt(l.marginLeft)+parseInt(l.marginRight)+te(s).width;if(r.display==="flex")return r.flexDirection==="column"||r.flexDirection==="column-reverse"?"vertical":"horizontal";if(r.display==="grid")return r.gridTemplateColumns.split(" ").length<=1?"vertical":"horizontal";if(n&&a.float&&a.float!=="none"){var p=a.float==="left"?"left":"right";return s&&(l.clear==="both"||l.clear===p)?"vertical":"horizontal"}return n&&(a.display==="block"||a.display==="flex"||a.display==="table"||a.display==="grid"||c>=o&&r[Od]==="none"||s&&r[Od]==="none"&&c+d>o)?"vertical":"horizontal"},g_=function(t,i,r){var o=r?t.left:t.top,n=r?t.right:t.bottom,s=r?t.width:t.height,a=r?i.left:i.top,l=r?i.right:i.bottom,c=r?i.width:i.height;return o===a||n===l||o+s/2===a+c/2},m_=function(t,i){var r;return xs.some(function(o){var n=o[we].options.emptyInsertThreshold;if(!(!n||lc(o))){var s=te(o),a=t>=s.left-n&&t<=s.right+n,l=i>=s.top-n&&i<=s.bottom+n;if(a&&l)return r=o}}),r},Bp=function(t){function i(n,s){return function(a,l,c,d){var p=a.options.group.name&&l.options.group.name&&a.options.group.name===l.options.group.name;if(n==null&&(s||p))return!0;if(n==null||n===!1)return!1;if(s&&n==="clone")return n;if(typeof n=="function")return i(n(a,l,c,d),s)(a,l,c,d);var f=(s?a:l).options.group.name;return n===!0||typeof n=="string"&&n===f||n.join&&n.indexOf(f)>-1}}var r={},o=t.group;(!o||sl(o)!="object")&&(o={name:o}),r.name=o.name,r.checkPull=i(o.pull,!0),r.checkPut=i(o.put),r.revertClone=o.revertClone,t.group=r},Up=function(){!Rp&&A&&S(A,"display","none")},jp=function(){!Rp&&A&&S(A,"display","")};Zs&&!Op&&document.addEventListener("click",function(e){if(ys)return e.preventDefault(),e.stopPropagation&&e.stopPropagation(),e.stopImmediatePropagation&&e.stopImmediatePropagation(),ys=!1,!1},!0);var Oi=function(t){if($){t=t.touches?t.touches[0]:t;var i=m_(t.clientX,t.clientY);if(i){var r={};for(var o in t)t.hasOwnProperty(o)&&(r[o]=t[o]);r.target=r.rootEl=i,r.preventDefault=void 0,r.stopPropagation=void 0,i[we]._onDragOver(r)}}},b_=function(t){$&&$.parentNode[we]._isOutsideThisEl(t.target)};function E(e,t){if(!(e&&e.nodeType&&e.nodeType===1))throw"Sortable: `el` must be an HTMLElement, not ".concat({}.toString.call(e));this.el=e,this.options=t=Ut({},t),e[we]=this;var i={group:null,sort:!0,disabled:!1,store:null,handle:null,draggable:/^[uo]l$/i.test(e.nodeName)?">li":">*",swapThreshold:1,invertSwap:!1,invertedSwapThreshold:null,removeCloneOnHide:!0,direction:function(){return Fp(e,this.options)},ghostClass:"sortable-ghost",chosenClass:"sortable-chosen",dragClass:"sortable-drag",ignore:"a, img",filter:null,preventOnFilter:!0,animation:0,easing:null,setData:function(s,a){s.setData("Text",a.textContent)},dropBubble:!1,dragoverBubble:!1,dataIdAttr:"data-id",delay:0,delayOnTouchOnly:!1,touchStartThreshold:(Number.parseInt?Number:window).parseInt(window.devicePixelRatio,10)||1,forceFallback:!1,fallbackClass:"sortable-fallback",fallbackOnBody:!1,fallbackTolerance:0,fallbackOffset:{x:0,y:0},supportPointer:E.supportPointer!==!1&&"PointerEvent"in window&&(!Lo||ac),emptyInsertThreshold:5};wn.initializePlugins(this,e,i);for(var r in i)!(r in t)&&(t[r]=i[r]);Bp(t);for(var o in this)o.charAt(0)==="_"&&typeof this[o]=="function"&&(this[o]=this[o].bind(this));this.nativeDraggable=t.forceFallback?!1:f_,this.nativeDraggable&&(this.options.touchStartThreshold=1),t.supportPointer?D(e,"pointerdown",this._onTapStart):(D(e,"mousedown",this._onTapStart),D(e,"touchstart",this._onTapStart)),this.nativeDraggable&&(D(e,"dragover",this),D(e,"dragenter",this)),xs.push(this.el),t.store&&t.store.get&&this.sort(t.store.get(this)||[]),Ut(this,c_())}E.prototype={constructor:E,_isOutsideThisEl:function(t){!this.el.contains(t)&&t!==this.el&&(wr=null)},_getDirection:function(t,i){return typeof this.options.direction=="function"?this.options.direction.call(this,t,i,$):this.options.direction},_onTapStart:function(t){if(t.cancelable){var i=this,r=this.el,o=this.options,n=o.preventOnFilter,s=t.type,a=t.touches&&t.touches[0]||t.pointerType&&t.pointerType==="touch"&&t,l=(a||t).target,c=t.target.shadowRoot&&(t.path&&t.path[0]||t.composedPath&&t.composedPath()[0])||l,d=o.filter;if(C_(r),!$&&!(/mousedown|pointerdown/.test(s)&&t.button!==0||o.disabled)&&!c.isContentEditable&&!(!this.nativeDraggable&&Lo&&l&&l.tagName.toUpperCase()==="SELECT")&&(l=Qe(l,o.draggable,r,!1),!(l&&l.animated)&&rs!==l)){if(Pr=Ve(l),Io=Ve(l,o.draggable),typeof d=="function"){if(d.call(this,t,l,this)){he({sortable:i,rootEl:c,name:"filter",targetEl:l,toEl:r,fromEl:r}),ye("filter",i,{evt:t}),n&&t.preventDefault();return}}else if(d&&(d=d.split(",").some(function(p){if(p=Qe(c,p.trim(),r,!1),p)return he({sortable:i,rootEl:p,name:"filter",targetEl:l,fromEl:r,toEl:r}),ye("filter",i,{evt:t}),!0}),d)){n&&t.preventDefault();return}o.handle&&!Qe(c,o.handle,r,!1)||this._prepareDragStart(t,a,l)}}},_prepareDragStart:function(t,i,r){var o=this,n=o.el,s=o.options,a=n.ownerDocument,l;if(r&&!$&&r.parentNode===n){var c=te(r);if(U=n,$=r,Z=$.parentNode,Li=$.nextSibling,rs=r,Nn=s.group,E.dragged=$,Mi={target:$,clientX:(i||t).clientX,clientY:(i||t).clientY},Td=Mi.clientX-c.left,Md=Mi.clientY-c.top,this._lastX=(i||t).clientX,this._lastY=(i||t).clientY,$.style["will-change"]="all",l=function(){if(ye("delayEnded",o,{evt:t}),E.eventCanceled){o._onDrop();return}o._disableDelayedDragEvents(),!Sd&&o.nativeDraggable&&($.draggable=!0),o._triggerDragStart(t,i),he({sortable:o,name:"choose",originalEvent:t}),Pe($,s.chosenClass,!0)},s.ignore.split(",").forEach(function(d){Ip($,d.trim(),Ea)}),D(a,"dragover",Oi),D(a,"mousemove",Oi),D(a,"touchmove",Oi),s.supportPointer?(D(a,"pointerup",o._onDrop),!this.nativeDraggable&&D(a,"pointercancel",o._onDrop)):(D(a,"mouseup",o._onDrop),D(a,"touchend",o._onDrop),D(a,"touchcancel",o._onDrop)),Sd&&this.nativeDraggable&&(this.options.touchStartThreshold=4,$.draggable=!0),ye("delayStart",this,{evt:t}),s.delay&&(!s.delayOnTouchOnly||i)&&(!this.nativeDraggable||!(xn||Xt))){if(E.eventCanceled){this._onDrop();return}s.supportPointer?(D(a,"pointerup",o._disableDelayedDrag),D(a,"pointercancel",o._disableDelayedDrag)):(D(a,"mouseup",o._disableDelayedDrag),D(a,"touchend",o._disableDelayedDrag),D(a,"touchcancel",o._disableDelayedDrag)),D(a,"mousemove",o._delayedDragTouchMoveHandler),D(a,"touchmove",o._delayedDragTouchMoveHandler),s.supportPointer&&D(a,"pointermove",o._delayedDragTouchMoveHandler),o._dragStartTimer=setTimeout(l,s.delay)}else l()}},_delayedDragTouchMoveHandler:function(t){var i=t.touches?t.touches[0]:t;Math.max(Math.abs(i.clientX-this._lastX),Math.abs(i.clientY-this._lastY))>=Math.floor(this.options.touchStartThreshold/(this.nativeDraggable&&window.devicePixelRatio||1))&&this._disableDelayedDrag()},_disableDelayedDrag:function(){$&&Ea($),clearTimeout(this._dragStartTimer),this._disableDelayedDragEvents()},_disableDelayedDragEvents:function(){var t=this.el.ownerDocument;L(t,"mouseup",this._disableDelayedDrag),L(t,"touchend",this._disableDelayedDrag),L(t,"touchcancel",this._disableDelayedDrag),L(t,"pointerup",this._disableDelayedDrag),L(t,"pointercancel",this._disableDelayedDrag),L(t,"mousemove",this._delayedDragTouchMoveHandler),L(t,"touchmove",this._delayedDragTouchMoveHandler),L(t,"pointermove",this._delayedDragTouchMoveHandler)},_triggerDragStart:function(t,i){i=i||t.pointerType=="touch"&&t,!this.nativeDraggable||i?this.options.supportPointer?D(document,"pointermove",this._onTouchMove):i?D(document,"touchmove",this._onTouchMove):D(document,"mousemove",this._onTouchMove):(D($,"dragend",this),D(U,"dragstart",this._onDragStart));try{document.selection?ns(function(){document.selection.empty()}):window.getSelection().removeAllRanges()}catch{}},_dragStarted:function(t,i){if(Er=!1,U&&$){ye("dragStarted",this,{evt:i}),this.nativeDraggable&&D(document,"dragover",b_);var r=this.options;!t&&Pe($,r.dragClass,!1),Pe($,r.ghostClass,!0),E.active=this,t&&this._appendGhost(),he({sortable:this,name:"start",originalEvent:i})}else this._nulling()},_emulateDragOver:function(){if(Ze){this._lastX=Ze.clientX,this._lastY=Ze.clientY,Up();for(var t=document.elementFromPoint(Ze.clientX,Ze.clientY),i=t;t&&t.shadowRoot&&(t=t.shadowRoot.elementFromPoint(Ze.clientX,Ze.clientY),t!==i);)i=t;if($.parentNode[we]._isOutsideThisEl(t),i)do{if(i[we]){var r=void 0;if(r=i[we]._onDragOver({clientX:Ze.clientX,clientY:Ze.clientY,target:t,rootEl:i}),r&&!this.options.dragoverBubble)break}t=i}while(i=Dp(i));jp()}},_onTouchMove:function(t){if(Mi){var i=this.options,r=i.fallbackTolerance,o=i.fallbackOffset,n=t.touches?t.touches[0]:t,s=A&&Mr(A,!0),a=A&&s&&s.a,l=A&&s&&s.d,c=Fn&&ae&&Ad(ae),d=(n.clientX-Mi.clientX+o.x)/(a||1)+(c?c[0]-Sa[0]:0)/(a||1),p=(n.clientY-Mi.clientY+o.y)/(l||1)+(c?c[1]-Sa[1]:0)/(l||1);if(!E.active&&!Er){if(r&&Math.max(Math.abs(n.clientX-this._lastX),Math.abs(n.clientY-this._lastY))<r)return;this._onDragStart(t,!0)}if(A){s?(s.e+=d-(ka||0),s.f+=p-(Ca||0)):s={a:1,b:0,c:0,d:1,e:d,f:p};var f="matrix(".concat(s.a,",").concat(s.b,",").concat(s.c,",").concat(s.d,",").concat(s.e,",").concat(s.f,")");S(A,"webkitTransform",f),S(A,"mozTransform",f),S(A,"msTransform",f),S(A,"transform",f),ka=d,Ca=p,Ze=n}t.cancelable&&t.preventDefault()}},_appendGhost:function(){if(!A){var t=this.options.fallbackOnBody?document.body:U,i=te($,!0,Fn,!0,t),r=this.options;if(Fn){for(ae=t;S(ae,"position")==="static"&&S(ae,"transform")==="none"&&ae!==document;)ae=ae.parentNode;ae!==document.body&&ae!==document.documentElement?(ae===document&&(ae=pt()),i.top+=ae.scrollTop,i.left+=ae.scrollLeft):ae=pt(),Sa=Ad(ae)}A=$.cloneNode(!0),Pe(A,r.ghostClass,!1),Pe(A,r.fallbackClass,!0),Pe(A,r.dragClass,!0),S(A,"transition",""),S(A,"transform",""),S(A,"box-sizing","border-box"),S(A,"margin",0),S(A,"top",i.top),S(A,"left",i.left),S(A,"width",i.width),S(A,"height",i.height),S(A,"opacity","0.8"),S(A,"position",Fn?"absolute":"fixed"),S(A,"zIndex","100000"),S(A,"pointerEvents","none"),E.ghost=A,t.appendChild(A),S(A,"transform-origin",Td/parseInt(A.style.width)*100+"% "+Md/parseInt(A.style.height)*100+"%")}},_onDragStart:function(t,i){var r=this,o=t.dataTransfer,n=r.options;if(ye("dragStart",this,{evt:t}),E.eventCanceled){this._onDrop();return}ye("setupClone",this),E.eventCanceled||(X=Vp($),X.removeAttribute("id"),X.draggable=!1,X.style["will-change"]="",this._hideClone(),Pe(X,this.options.chosenClass,!1),E.clone=X),r.cloneId=ns(function(){ye("clone",r),!E.eventCanceled&&(r.options.removeCloneOnHide||U.insertBefore(X,$),r._hideClone(),he({sortable:r,name:"clone"}))}),!i&&Pe($,n.dragClass,!0),i?(ys=!0,r._loopId=setInterval(r._emulateDragOver,50)):(L(document,"mouseup",r._onDrop),L(document,"touchend",r._onDrop),L(document,"touchcancel",r._onDrop),o&&(o.effectAllowed="move",n.setData&&n.setData.call(r,o,$)),D(document,"drop",r),S($,"transform","translateZ(0)")),Er=!0,r._dragStartId=ns(r._dragStarted.bind(r,i,t)),D(document,"selectstart",r),Eo=!0,window.getSelection().removeAllRanges(),Lo&&S(document.body,"user-select","none")},_onDragOver:function(t){var i=this.el,r=t.target,o,n,s,a=this.options,l=a.group,c=E.active,d=Nn===l,p=a.sort,f=se||c,b,v=this,_=!1;if(al)return;function k(go,Mu){ye(go,v,vt({evt:t,isOwner:d,axis:b?"vertical":"horizontal",revert:s,dragRect:o,targetRect:n,canSort:p,fromSortable:f,target:r,completed:O,onMove:function(Ec,Ou){return Bn(U,i,$,o,Ec,te(Ec),t,Ou)},changed:I},Mu))}function C(){k("dragOverAnimationCapture"),v.captureAnimationState(),v!==f&&f.captureAnimationState()}function O(go){return k("dragOverCompleted",{insertion:go}),go&&(d?c._hideClone():c._showClone(v),v!==f&&(Pe($,se?se.options.ghostClass:c.options.ghostClass,!1),Pe($,a.ghostClass,!0)),se!==v&&v!==E.active?se=v:v===E.active&&se&&(se=null),f===v&&(v._ignoreWhileAnimating=r),v.animateAll(function(){k("dragOverAnimationComplete"),v._ignoreWhileAnimating=null}),v!==f&&(f.animateAll(),f._ignoreWhileAnimating=null)),(r===$&&!$.animated||r===i&&!r.animated)&&(wr=null),!a.dragoverBubble&&!t.rootEl&&r!==document&&($.parentNode[we]._isOutsideThisEl(t.target),!go&&Oi(t)),!a.dragoverBubble&&t.stopPropagation&&t.stopPropagation(),_=!0}function I(){Ae=Ve($),ti=Ve($,a.draggable),he({sortable:v,name:"change",toEl:i,newIndex:Ae,newDraggableIndex:ti,originalEvent:t})}if(t.preventDefault!==void 0&&t.cancelable&&t.preventDefault(),r=Qe(r,a.draggable,i,!0),k("dragOver"),E.eventCanceled)return _;if($.contains(t.target)||r.animated&&r.animatingX&&r.animatingY||v._ignoreWhileAnimating===r)return O(!1);if(ys=!1,c&&!a.disabled&&(d?p||(s=Z!==U):se===this||(this.lastPutMode=Nn.checkPull(this,c,$,t))&&l.checkPut(this,c,$,t))){if(b=this._getDirection(t,r)==="vertical",o=te($),k("dragOverValid"),E.eventCanceled)return _;if(s)return Z=U,C(),this._hideClone(),k("revert"),E.eventCanceled||(Li?U.insertBefore($,Li):U.appendChild($)),O(!0);var V=lc(i,a.draggable);if(!V||x_(t,b,this)&&!V.animated){if(V===$)return O(!1);if(V&&i===t.target&&(r=V),r&&(n=te(r)),Bn(U,i,$,o,r,n,t,!!r)!==!1)return C(),V&&V.nextSibling?i.insertBefore($,V.nextSibling):i.appendChild($),Z=i,I(),O(!0)}else if(V&&y_(t,b,this)){var K=Zr(i,0,a,!0);if(K===$)return O(!1);if(r=K,n=te(r),Bn(U,i,$,o,r,n,t,!1)!==!1)return C(),i.insertBefore($,K),Z=i,I(),O(!0)}else if(r.parentNode===i){n=te(r);var R=0,Q,de=$.parentNode!==i,G=!g_($.animated&&$.toRect||o,r.animated&&r.toRect||n,b),Ie=b?"top":"left",_e=Pd(r,"top","top")||Pd($,"top","top"),uo=_e?_e.scrollTop:void 0;wr!==r&&(Q=n[Ie],zo=!1,Rn=!G&&a.invertSwap||de),R=w_(t,r,n,b,G?1:a.swapThreshold,a.invertedSwapThreshold==null?a.swapThreshold:a.invertedSwapThreshold,Rn,wr===r);var Ot;if(R!==0){var Pi=Ve($);do Pi-=R,Ot=Z.children[Pi];while(Ot&&(S(Ot,"display")==="none"||Ot===A))}if(R===0||Ot===r)return O(!1);wr=r,Ho=R;var fo=r.nextElementSibling,Zt=!1;Zt=R===1;var Mn=Bn(U,i,$,o,r,n,t,Zt);if(Mn!==!1)return(Mn===1||Mn===-1)&&(Zt=Mn===1),al=!0,setTimeout(__,30),C(),Zt&&!fo?i.appendChild($):r.parentNode.insertBefore($,Zt?fo:r),_e&&zp(_e,0,uo-_e.scrollTop),Z=$.parentNode,Q!==void 0&&!Rn&&(os=Math.abs(Q-te(r)[Ie])),I(),O(!0)}if(i.contains($))return O(!1)}return!1},_ignoreWhileAnimating:null,_offMoveEvents:function(){L(document,"mousemove",this._onTouchMove),L(document,"touchmove",this._onTouchMove),L(document,"pointermove",this._onTouchMove),L(document,"dragover",Oi),L(document,"mousemove",Oi),L(document,"touchmove",Oi)},_offUpEvents:function(){var t=this.el.ownerDocument;L(t,"mouseup",this._onDrop),L(t,"touchend",this._onDrop),L(t,"pointerup",this._onDrop),L(t,"pointercancel",this._onDrop),L(t,"touchcancel",this._onDrop),L(document,"selectstart",this)},_onDrop:function(t){var i=this.el,r=this.options;if(Ae=Ve($),ti=Ve($,r.draggable),ye("drop",this,{evt:t}),Z=$&&$.parentNode,Ae=Ve($),ti=Ve($,r.draggable),E.eventCanceled){this._nulling();return}Er=!1,Rn=!1,zo=!1,clearInterval(this._loopId),clearTimeout(this._dragStartTimer),ll(this.cloneId),ll(this._dragStartId),this.nativeDraggable&&(L(document,"drop",this),L(i,"dragstart",this._onDragStart)),this._offMoveEvents(),this._offUpEvents(),Lo&&S(document.body,"user-select",""),S($,"transform",""),t&&(Eo&&(t.cancelable&&t.preventDefault(),!r.dropBubble&&t.stopPropagation()),A&&A.parentNode&&A.parentNode.removeChild(A),(U===Z||se&&se.lastPutMode!=="clone")&&X&&X.parentNode&&X.parentNode.removeChild(X),$&&(this.nativeDraggable&&L($,"dragend",this),Ea($),$.style["will-change"]="",Eo&&!Er&&Pe($,se?se.options.ghostClass:this.options.ghostClass,!1),Pe($,this.options.chosenClass,!1),he({sortable:this,name:"unchoose",toEl:Z,newIndex:null,newDraggableIndex:null,originalEvent:t}),U!==Z?(Ae>=0&&(he({rootEl:Z,name:"add",toEl:Z,fromEl:U,originalEvent:t}),he({sortable:this,name:"remove",toEl:Z,originalEvent:t}),he({rootEl:Z,name:"sort",toEl:Z,fromEl:U,originalEvent:t}),he({sortable:this,name:"sort",toEl:Z,originalEvent:t})),se&&se.save()):Ae!==Pr&&Ae>=0&&(he({sortable:this,name:"update",toEl:Z,originalEvent:t}),he({sortable:this,name:"sort",toEl:Z,originalEvent:t})),E.active&&((Ae==null||Ae===-1)&&(Ae=Pr,ti=Io),he({sortable:this,name:"end",toEl:Z,originalEvent:t}),this.save()))),this._nulling()},_nulling:function(){ye("nulling",this),U=$=Z=A=Li=X=rs=ri=Mi=Ze=Eo=Ae=ti=Pr=Io=wr=Ho=se=Nn=E.dragged=E.ghost=E.clone=E.active=null;var t=this.el;ws.forEach(function(i){t.contains(i)&&(i.checked=!0)}),ws.length=ka=Ca=0},handleEvent:function(t){switch(t.type){case"drop":case"dragend":this._onDrop(t);break;case"dragenter":case"dragover":$&&(this._onDragOver(t),v_(t));break;case"selectstart":t.preventDefault();break}},toArray:function(){for(var t=[],i,r=this.el.children,o=0,n=r.length,s=this.options;o<n;o++)i=r[o],Qe(i,s.draggable,this.el,!1)&&t.push(i.getAttribute(s.dataIdAttr)||k_(i));return t},sort:function(t,i){var r={},o=this.el;this.toArray().forEach(function(n,s){var a=o.children[s];Qe(a,this.options.draggable,o,!1)&&(r[n]=a)},this),i&&this.captureAnimationState(),t.forEach(function(n){r[n]&&(o.removeChild(r[n]),o.appendChild(r[n]))}),i&&this.animateAll()},save:function(){var t=this.options.store;t&&t.set&&t.set(this)},closest:function(t,i){return Qe(t,i||this.options.draggable,this.el,!1)},option:function(t,i){var r=this.options;if(i===void 0)return r[t];var o=wn.modifyOption(this,t,i);typeof o<"u"?r[t]=o:r[t]=i,t==="group"&&Bp(r)},destroy:function(){ye("destroy",this);var t=this.el;t[we]=null,L(t,"mousedown",this._onTapStart),L(t,"touchstart",this._onTapStart),L(t,"pointerdown",this._onTapStart),this.nativeDraggable&&(L(t,"dragover",this),L(t,"dragenter",this)),Array.prototype.forEach.call(t.querySelectorAll("[draggable]"),function(i){i.removeAttribute("draggable")}),this._onDrop(),this._disableDelayedDragEvents(),xs.splice(xs.indexOf(this.el),1),this.el=t=null},_hideClone:function(){if(!ri){if(ye("hideClone",this),E.eventCanceled)return;S(X,"display","none"),this.options.removeCloneOnHide&&X.parentNode&&X.parentNode.removeChild(X),ri=!0}},_showClone:function(t){if(t.lastPutMode!=="clone"){this._hideClone();return}if(ri){if(ye("showClone",this),E.eventCanceled)return;$.parentNode==U&&!this.options.group.revertClone?U.insertBefore(X,$):Li?U.insertBefore(X,Li):U.appendChild(X),this.options.group.revertClone&&this.animate($,X),S(X,"display",""),ri=!1}}};function v_(e){e.dataTransfer&&(e.dataTransfer.dropEffect="move"),e.cancelable&&e.preventDefault()}function Bn(e,t,i,r,o,n,s,a){var l,c=e[we],d=c.options.onMove,p;return window.CustomEvent&&!Xt&&!xn?l=new CustomEvent("move",{bubbles:!0,cancelable:!0}):(l=document.createEvent("Event"),l.initEvent("move",!0,!0)),l.to=t,l.from=e,l.dragged=i,l.draggedRect=r,l.related=o||t,l.relatedRect=n||te(t),l.willInsertAfter=a,l.originalEvent=s,e.dispatchEvent(l),d&&(p=d.call(c,l,s)),p}function Ea(e){e.draggable=!1}function __(){al=!1}function y_(e,t,i){var r=te(Zr(i.el,0,i.options,!0)),o=Np(i.el,i.options,A),n=10;return t?e.clientX<o.left-n||e.clientY<r.top&&e.clientX<r.right:e.clientY<o.top-n||e.clientY<r.bottom&&e.clientX<r.left}function x_(e,t,i){var r=te(lc(i.el,i.options.draggable)),o=Np(i.el,i.options,A),n=10;return t?e.clientX>o.right+n||e.clientY>r.bottom&&e.clientX>r.left:e.clientY>o.bottom+n||e.clientX>r.right&&e.clientY>r.top}function w_(e,t,i,r,o,n,s,a){var l=r?e.clientY:e.clientX,c=r?i.height:i.width,d=r?i.top:i.left,p=r?i.bottom:i.right,f=!1;if(!s){if(a&&os<c*o){if(!zo&&(Ho===1?l>d+c*n/2:l<p-c*n/2)&&(zo=!0),zo)f=!0;else if(Ho===1?l<d+os:l>p-os)return-Ho}else if(l>d+c*(1-o)/2&&l<p-c*(1-o)/2)return $_(t)}return f=f||s,f&&(l<d+c*n/2||l>p-c*n/2)?l>d+c/2?1:-1:0}function $_(e){return Ve($)<Ve(e)?1:-1}function k_(e){for(var t=e.tagName+e.className+e.src+e.href+e.textContent,i=t.length,r=0;i--;)r+=t.charCodeAt(i);return r.toString(36)}function C_(e){ws.length=0;for(var t=e.getElementsByTagName("input"),i=t.length;i--;){var r=t[i];r.checked&&ws.push(r)}}function ns(e){return setTimeout(e,0)}function ll(e){return clearTimeout(e)}Zs&&D(document,"touchmove",function(e){(E.active||Er)&&e.cancelable&&e.preventDefault()});E.utils={on:D,off:L,css:S,find:Ip,is:function(t,i){return!!Qe(t,i,t,!1)},extend:a_,throttle:Hp,closest:Qe,toggleClass:Pe,clone:Vp,index:Ve,nextTick:ns,cancelNextTick:ll,detectDirection:Fp,getChild:Zr,expando:we};E.get=function(e){return e[we]};E.mount=function(){for(var e=arguments.length,t=new Array(e),i=0;i<e;i++)t[i]=arguments[i];t[0].constructor===Array&&(t=t[0]),t.forEach(function(r){if(!r.prototype||!r.prototype.constructor)throw"Sortable: Mounted plugin must be a constructor function, not ".concat({}.toString.call(r));r.utils&&(E.utils=vt(vt({},E.utils),r.utils)),wn.mount(r)})};E.create=function(e,t){return new E(e,t)};E.version=n_;var ee=[],Po,cl,dl=!1,Pa,Aa,$s,Ao;function S_(){function e(){this.defaults={scroll:!0,forceAutoScrollFallback:!1,scrollSensitivity:30,scrollSpeed:10,bubbleScroll:!0};for(var t in this)t.charAt(0)==="_"&&typeof this[t]=="function"&&(this[t]=this[t].bind(this))}return e.prototype={dragStarted:function(i){var r=i.originalEvent;this.sortable.nativeDraggable?D(document,"dragover",this._handleAutoScroll):this.options.supportPointer?D(document,"pointermove",this._handleFallbackAutoScroll):r.touches?D(document,"touchmove",this._handleFallbackAutoScroll):D(document,"mousemove",this._handleFallbackAutoScroll)},dragOverCompleted:function(i){var r=i.originalEvent;!this.options.dragOverBubble&&!r.rootEl&&this._handleAutoScroll(r)},drop:function(){this.sortable.nativeDraggable?L(document,"dragover",this._handleAutoScroll):(L(document,"pointermove",this._handleFallbackAutoScroll),L(document,"touchmove",this._handleFallbackAutoScroll),L(document,"mousemove",this._handleFallbackAutoScroll)),Ld(),ss(),l_()},nulling:function(){$s=cl=Po=dl=Ao=Pa=Aa=null,ee.length=0},_handleFallbackAutoScroll:function(i){this._handleAutoScroll(i,!0)},_handleAutoScroll:function(i,r){var o=this,n=(i.touches?i.touches[0]:i).clientX,s=(i.touches?i.touches[0]:i).clientY,a=document.elementFromPoint(n,s);if($s=i,r||this.options.forceAutoScrollFallback||xn||Xt||Lo){Ta(i,this.options,a,r);var l=si(a,!0);dl&&(!Ao||n!==Pa||s!==Aa)&&(Ao&&Ld(),Ao=setInterval(function(){var c=si(document.elementFromPoint(n,s),!0);c!==l&&(l=c,ss()),Ta(i,o.options,c,r)},10),Pa=n,Aa=s)}else{if(!this.options.bubbleScroll||si(a,!0)===pt()){ss();return}Ta(i,this.options,si(a,!1),!1)}}},Ut(e,{pluginName:"scroll",initializeByDefault:!0})}function ss(){ee.forEach(function(e){clearInterval(e.pid)}),ee=[]}function Ld(){clearInterval(Ao)}var Ta=Hp(function(e,t,i,r){if(t.scroll){var o=(e.touches?e.touches[0]:e).clientX,n=(e.touches?e.touches[0]:e).clientY,s=t.scrollSensitivity,a=t.scrollSpeed,l=pt(),c=!1,d;cl!==i&&(cl=i,ss(),Po=t.scroll,d=t.scrollFn,Po===!0&&(Po=si(i,!0)));var p=0,f=Po;do{var b=f,v=te(b),_=v.top,k=v.bottom,C=v.left,O=v.right,I=v.width,V=v.height,K=void 0,R=void 0,Q=b.scrollWidth,de=b.scrollHeight,G=S(b),Ie=b.scrollLeft,_e=b.scrollTop;b===l?(K=I<Q&&(G.overflowX==="auto"||G.overflowX==="scroll"||G.overflowX==="visible"),R=V<de&&(G.overflowY==="auto"||G.overflowY==="scroll"||G.overflowY==="visible")):(K=I<Q&&(G.overflowX==="auto"||G.overflowX==="scroll"),R=V<de&&(G.overflowY==="auto"||G.overflowY==="scroll"));var uo=K&&(Math.abs(O-o)<=s&&Ie+I<Q)-(Math.abs(C-o)<=s&&!!Ie),Ot=R&&(Math.abs(k-n)<=s&&_e+V<de)-(Math.abs(_-n)<=s&&!!_e);if(!ee[p])for(var Pi=0;Pi<=p;Pi++)ee[Pi]||(ee[Pi]={});(ee[p].vx!=uo||ee[p].vy!=Ot||ee[p].el!==b)&&(ee[p].el=b,ee[p].vx=uo,ee[p].vy=Ot,clearInterval(ee[p].pid),(uo!=0||Ot!=0)&&(c=!0,ee[p].pid=setInterval((function(){r&&this.layer===0&&E.active._onTouchMove($s);var fo=ee[this.layer].vy?ee[this.layer].vy*a:0,Zt=ee[this.layer].vx?ee[this.layer].vx*a:0;typeof d=="function"&&d.call(E.dragged.parentNode[we],Zt,fo,e,$s,ee[this.layer].el)!=="continue"||zp(ee[this.layer].el,Zt,fo)}).bind({layer:p}),24))),p++}while(t.bubbleScroll&&f!==l&&(f=si(f,!1)));dl=c}},30),qp=function(t){var i=t.originalEvent,r=t.putSortable,o=t.dragEl,n=t.activeSortable,s=t.dispatchSortableEvent,a=t.hideGhostForTarget,l=t.unhideGhostForTarget;if(i){var c=r||n;a();var d=i.changedTouches&&i.changedTouches.length?i.changedTouches[0]:i,p=document.elementFromPoint(d.clientX,d.clientY);l(),c&&!c.el.contains(p)&&(s("spill"),this.onSpill({dragEl:o,putSortable:r}))}};function cc(){}cc.prototype={startIndex:null,dragStart:function(t){var i=t.oldDraggableIndex;this.startIndex=i},onSpill:function(t){var i=t.dragEl,r=t.putSortable;this.sortable.captureAnimationState(),r&&r.captureAnimationState();var o=Zr(this.sortable.el,this.startIndex,this.options);o?this.sortable.el.insertBefore(i,o):this.sortable.el.appendChild(i),this.sortable.animateAll(),r&&r.animateAll()},drop:qp};Ut(cc,{pluginName:"revertOnSpill"});function dc(){}dc.prototype={onSpill:function(t){var i=t.dragEl,r=t.putSortable,o=r||this.sortable;o.captureAnimationState(),i.parentNode&&i.parentNode.removeChild(i),o.animateAll()},drop:qp};Ut(dc,{pluginName:"removeOnSpill"});E.mount(new S_);E.mount(dc,cc);var E_=Object.defineProperty,P_=Object.getOwnPropertyDescriptor,Et=(e,t,i,r)=>{for(var o=r>1?void 0:r?P_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&E_(t,i,o),o};let tt=class extends y{constructor(){super(...arguments),this.disabled=!1,this.noStyle=!1,this.invertSwap=!1,this.rollback=!0,this._shouldDestroy=!1,this._handleUpdate=e=>{Y(this,"item-moved",{newIndex:e.newIndex,oldIndex:e.oldIndex})},this._handleAdd=e=>{Y(this,"item-added",{index:e.newIndex,data:e.item.sortableData,item:e.item})},this._handleRemove=e=>{Y(this,"item-removed",{index:e.oldIndex})},this._handleEnd=e=>{if(Y(this,"drag-end"),this.rollback&&e.item.placeholder){const t=e.item;t.placeholder.replaceWith(t),delete t.placeholder}},this._handleStart=()=>{Y(this,"drag-start")},this._handleChoose=e=>{if(!this.rollback)return;const t=e.item;t.placeholder=document.createComment("sort-placeholder"),t.after(t.placeholder)}}updated(e){e.has("disabled")&&this._ensureSortable()}disconnectedCallback(){super.disconnectedCallback(),this._shouldDestroy=!0,setTimeout(()=>{this._shouldDestroy&&(this._destroySortable(),this._shouldDestroy=!1)},1)}connectedCallback(){super.connectedCallback(),this._shouldDestroy=!1,this.hasUpdated&&!this._sortable&&!this.disabled&&this._ensureSortable()}createRenderRoot(){return this}render(){return this.noStyle?m:h`
      <style>
        .sortable-fallback {
          display: none !important;
        }
        .sortable-ghost {
          box-shadow: 0 0 0 2px var(--primary-color);
          background: rgba(3, 169, 244, 0.2);
          border-radius: var(--ha-card-border-radius, 12px);
          opacity: 0.45;
        }
        .sortable-drag {
          border-radius: var(--ha-card-border-radius, 12px);
          opacity: 1;
          background: var(--card-background-color);
          box-shadow: 0 4px 12px rgba(0, 0, 0, 0.18);
          cursor: grabbing;
        }
      </style>
    `}firstUpdated(){this.disabled||this._ensureSortable()}async _ensureSortable(){if(this._sortable){this._sortable.option("disabled",this.disabled);return}this.disabled||await this._createSortable()}async _createSortable(){if(this._sortable)return;const e=this.children[0];if(!e)return;const t={scroll:!0,forceAutoScrollFallback:!0,scrollSpeed:20,animation:150,disabled:this.disabled,...this.options,onChoose:this._handleChoose,onStart:this._handleStart,onEnd:this._handleEnd,onUpdate:this._handleUpdate,onAdd:this._handleAdd,onRemove:this._handleRemove};this.draggableSelector&&(t.draggable=this.draggableSelector),this.handleSelector&&(t.handle=this.handleSelector),this.invertSwap!==void 0&&(t.invertSwap=this.invertSwap),this.group&&(t.group=this.group),this.filter&&(t.filter=this.filter),this._sortable=E.create(e,t)}_destroySortable(){var e;(e=this._sortable)==null||e.destroy(),this._sortable=void 0}};Et([u({type:Boolean})],tt.prototype,"disabled",2);Et([u({type:Boolean,attribute:"no-style"})],tt.prototype,"noStyle",2);Et([u({type:String,attribute:"draggable-selector"})],tt.prototype,"draggableSelector",2);Et([u({type:String,attribute:"handle-selector"})],tt.prototype,"handleSelector",2);Et([u({type:String})],tt.prototype,"filter",2);Et([u({attribute:!1})],tt.prototype,"group",2);Et([u({type:Boolean,attribute:"invert-swap"})],tt.prototype,"invertSwap",2);Et([u({attribute:!1})],tt.prototype,"options",2);Et([u({type:Boolean})],tt.prototype,"rollback",2);tt=Et([x("ha-sortable")],tt);function A_(e,t,i){if(!t.startsWith("/demo/"))return;const r=e.getState(t);if(r){if(t.includes("climate")||t.includes("thermostat")||t==="/demo/ac"){typeof i=="number"&&(typeof r.state=="number"?e.applyUpdate(t,i,r.attributes):e.applyUpdate(t,r.state,{...r.attributes,temperature:i}));return}if(t.includes("gain")&&typeof i=="number"){e.applyUpdate(t,i,r.attributes);return}if(t.includes("hsv")){const o=ec(i);o&&e.applyUpdate(t,mp(o),r.attributes);return}if(t.includes("rgba")||t.includes("color")){const o=Jl(i);o&&e.applyUpdate(t,fp(o),r.attributes);return}if(t.includes("fader")||t.includes("vector")){if(Array.isArray(i)){const o=_p(i,i.length,{min:-1/0,max:1/0,step:0});e.applyUpdate(t,yp(o),r.attributes)}return}if(t.includes("xy")){const o=ic(i);o?e.applyUpdate(t,xp(o),r.attributes):typeof i=="number"&&e.applyUpdate(t,i,r.attributes);return}if(t.includes("shutter")||t.includes("cover")){if(typeof i=="number"){const o=Math.round(i),n=o<=0;e.applyUpdate(t,n?"closed":"open",{...r.attributes,current_position:o})}return}if(t.includes("trigger")||t.includes("button")){e.applyUpdate(t,!0,r.attributes),window.setTimeout(()=>{const o=e.getState(t);e.applyUpdate(t,!1,(o==null?void 0:o.attributes)??r.attributes)},700);return}e.applyUpdate(t,i,r.attributes)}}function T_(e,t){let i=0;return e.subscribeAll(()=>{i+=1}),{get connected(){return t.connected},get states(){return e.getStatesRecord()},get statesRevision(){return i},connection:t,subscribeStates(r){return t.subscribeStates(r)},subscribeEntity(r,o){return e.subscribe(r,o)},async callService(r,o){t.sendCommand(r,o),t.connected||A_(e,r,o)},formatState(r){const o=e.getState(r);if(!o)return"—";const n=o.state;if(typeof n=="boolean")return n?"开启":"关闭";if(Array.isArray(n)){if(r.includes("hsv")&&n.length===3){const l=ec(n);if(l)return bp(l)}const a=Jl(n);return a?gp(a):n.join(", ")}if(n==null)return"—";const s=o.attributes.unit_of_measurement;return s?`${n} ${s}`:String(n)}}}function Gp(e){var n,s,a;if(!e)return[];const t=new Set;e.entity&&t.add(e.entity);const i=e.mute_entity;typeof i=="string"&&i&&t.add(i);const r=e.entity_x;typeof r=="string"&&r&&t.add(r);const o=e.entity_y;return typeof o=="string"&&o&&t.add(o),(n=e.badges)==null||n.forEach(l=>{l.entity&&t.add(l.entity)}),(s=e.elements)==null||s.forEach(l=>{l.entity&&t.add(l.entity)}),(a=e.entities)==null||a.forEach(l=>{l.entity&&t.add(l.entity)}),[...t]}function Dd(e,t){const i=document.createElement("hui-error-card");return i.setConfig({type:"error",message:e,origConfig:t}),i}async function Ks(e){let t=e;e.type==="button"?t={...e,type:"trigger"}:e.type==="color"&&(t={...e,type:"rgba"});try{const i=await ml(t.type);if(!i)return Dd(`Unknown card type: ${t.type}`,t);const r=new i;return r.setConfig(t),r}catch(i){const r=i instanceof Error?i.message:String(i);return Dd(r,e)}}const ai=3;function Ma(e,t){return typeof e=="number"?e*t:e}function Xp(e){const t={columns:Ma(e.grid_columns,ai),max_columns:Ma(e.grid_max_columns,ai),min_columns:Ma(e.grid_min_columns,ai),rows:e.grid_rows,max_rows:e.grid_max_rows,min_rows:e.grid_min_rows};for(const[i,r]of Object.entries(t))r===void 0&&delete t[i];return t}const Or={columns:12,rows:"auto"};function Id(e,t,i){let r=e;return t!==void 0&&(r=Math.max(r,t)),i!==void 0&&(r=Math.min(r,i)),r}function Wp(e={}){const t=e.rows??"auto",i=e.columns??12,r=e.min_rows,o=e.max_rows,n=e.min_columns,s=e.max_columns,a=typeof t=="string"?t:Id(t,r,o),l=typeof i=="string"?i:Id(i,n,s);return{rows:a,columns:l}}function M_(e){const t=e.columns==="full",i=e.rows==="auto",r=t?12:e.columns,o=i?1:e.rows;return{columnSize:r,rowSize:o,fullWidth:t,autoHeight:i}}function Hd(e){return typeof e.columns=="number"&&e.columns%ai!==0}const O_={columns:12,rows:"auto"};function L_(e){return e!=null&&e.grid_options?e.grid_options:e!=null&&e.layout_options?Xp(e.layout_options):{}}function D_(e={},t){const i=L_(t),r={...O_,...e,...i};return typeof e.min_rows=="number"&&typeof r.rows=="number"&&r.rows<e.min_rows&&(r.rows=e.min_rows),typeof e.min_columns=="number"&&typeof r.columns=="number"&&r.columns<e.min_columns&&(r.columns=e.min_columns),r}function I_(e,t){var i;return D_(((i=e==null?void 0:e.getGridOptions)==null?void 0:i.call(e))??{},t)}const hl="flow-edit-menu-open";function H_(e){document.dispatchEvent(new CustomEvent(hl,{bubbles:!0,composed:!0,detail:{source:e}}))}function hc(e){const t=i=>{var o;const r=(o=i.detail)==null?void 0:o.source;r&&e(r)};return document.addEventListener(hl,t),()=>document.removeEventListener(hl,t)}function z_(e,t,i={}){const r=i.margin??8,o=i.gap??4,n=i.preferredMaxHeight??360,s=i.minMenuWidth??160,a=Math.max(s,t.width),l=window.innerHeight-e.bottom-r,c=e.top-r,d=Math.max(t.height,1),p=l>=d+o||l>=c,f=p?"bottom-end":"top-end",b=p?l-o:c-o,v=Math.max(120,Math.min(n,b)),_=Math.min(d,v);let k=p?e.bottom+o:e.top-_-o;k=Math.max(r,k);let C=e.right-a;return C=Math.max(r,Math.min(C,window.innerWidth-a-r)),{top:k,left:C,width:a,maxHeight:v,placement:f}}function pc(e,t=320){const i=e.getBoundingClientRect(),r=8,o=4,n=Math.max(i.width,160),s=180,a=window.innerHeight-i.bottom-r,l=i.top-r,c=a>=s+o||a>=l,d=Math.max(120,Math.min(t,(c?a:l)-o)),p=Math.min(s,d);return{top:c?i.bottom+o:Math.max(r,i.top-p-o),left:i.left,width:n,maxHeight:d}}var V_=Object.defineProperty,N_=Object.getOwnPropertyDescriptor,Qs=(e,t,i,r)=>{for(var o=r>1?void 0:r?N_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&V_(t,i,o),o};const Oa=160;let Kr=class extends y{constructor(){super(...arguments),this.items=[],this._open=!1,this._menuStyle={},this._onDocumentClick=e=>{this._open&&(e.composedPath().includes(this)||this._close())},this._repositionMenu=()=>{this._open&&this._updateMenuPosition()}}connectedCallback(){super.connectedCallback(),document.addEventListener("click",this._onDocumentClick),window.addEventListener("resize",this._repositionMenu),this._unsubMenuOpen=hc(e=>{e!==this&&this._open&&this._close()})}disconnectedCallback(){var e;document.removeEventListener("click",this._onDocumentClick),window.removeEventListener("resize",this._repositionMenu),this._detachScrollListener(),(e=this._unsubMenuOpen)==null||e.call(this),super.disconnectedCallback()}_attachScrollListener(){this._detachScrollListener();let e=this.parentElement;for(;e;){const t=getComputedStyle(e);if(/(auto|scroll)/.test(t.overflowY)){this._scrollParent=e,e.addEventListener("scroll",this._repositionMenu,{passive:!0});return}e=e.parentElement}}_detachScrollListener(){var e;(e=this._scrollParent)==null||e.removeEventListener("scroll",this._repositionMenu),this._scrollParent=void 0}_measureMenu(e){const t=Math.max(e.offsetWidth,Oa),i=Math.max(e.scrollHeight,e.offsetHeight);return{width:t,height:i}}_updateMenuPosition(){var n,s;const e=(n=this.shadowRoot)==null?void 0:n.querySelector(".trigger"),t=(s=this.shadowRoot)==null?void 0:s.querySelector(".menu");if(!e||!t)return!1;const i=e.getBoundingClientRect();if(i.width===0&&i.height===0)return!1;const r=this._measureMenu(t),o=z_(i,r,{preferredMaxHeight:360,minMenuWidth:Oa});return this._menuStyle={position:"fixed",top:`${o.top}px`,left:`${o.left}px`,width:`${o.width}px`,maxHeight:`${o.maxHeight}px`,zIndex:"200",visibility:"visible"},!0}async _scheduleMenuPosition(){if(await this.updateComplete,!!this._open){if(!this._updateMenuPosition()){if(await new Promise(e=>requestAnimationFrame(()=>e())),!this._open)return;this._updateMenuPosition()}await new Promise(e=>requestAnimationFrame(()=>e())),this._open&&this._updateMenuPosition()}}updated(e){super.updated(e),e.has("_open")&&this._open&&this._scheduleMenuPosition()}_openMenu(e){if(e.stopPropagation(),this._open){this._close();return}H_(this),this._menuStyle={position:"fixed",visibility:"hidden",width:`${Oa}px`},this._open=!0,this.setAttribute("open",""),this._attachScrollListener(),this.dispatchEvent(new CustomEvent("menu-opened",{bubbles:!0,composed:!0}))}_close(){this._open&&(this._open=!1,this._menuStyle={},this.removeAttribute("open"),this._detachScrollListener(),this.dispatchEvent(new CustomEvent("menu-closed",{bubbles:!0,composed:!0})))}_select(e,t){t.stopPropagation(),this._close(),this.dispatchEvent(new CustomEvent("action",{bubbles:!0,composed:!0,detail:{value:e}}))}render(){return h`
      <div class="wrap">
        <button
          type="button"
          class="trigger"
          @click=${this._openMenu}
          aria-label="更多操作"
          aria-expanded=${this._open?"true":"false"}
        >
          <slot name="trigger">⋮</slot>
        </button>
        ${this._open?h`
              <div class="menu" style=${P(this._menuStyle)} role="menu" @click=${e=>e.stopPropagation()}>
                ${this.items.map(e=>e.divider?h`<div class="divider"></div>`:h`
                        <button
                          type="button"
                          class="item ${e.destructive?"destructive":""}"
                          @click=${t=>this._select(e.value,t)}
                        >
                          ${e.icon?e.icon.startsWith("mdi:")?h`<ha-icon class="icon" .icon=${e.icon}></ha-icon>`:h`<span class="icon">${e.icon}</span>`:m}
                          <span>${e.label}</span>
                        </button>
                      `)}
              </div>
            `:m}
      </div>
    `}};Kr.styles=w`
    :host {
      display: inline-flex;
    }
    :host([open]) {
      z-index: 200;
    }
    .wrap {
      position: relative;
      display: inline-flex;
    }
    .trigger {
      border: none;
      background: var(--ha-action-menu-trigger-background, transparent);
      cursor: pointer;
      color: inherit;
      font-size: 18px;
      line-height: 1;
      padding: 0;
      width: var(--ha-action-menu-trigger-size, auto);
      height: var(--ha-action-menu-trigger-size, auto);
      min-width: var(--ha-action-menu-trigger-size, auto);
      min-height: var(--ha-action-menu-trigger-size, auto);
      border-radius: var(--ha-action-menu-trigger-radius, 50%);
      display: inline-flex;
      align-items: center;
      justify-content: center;
    }
    .trigger:hover {
      background: var(
        --ha-action-menu-trigger-hover-background,
        rgba(0, 0, 0, 0.04)
      );
    }
    .menu {
      background: var(--card-background-color, #fff);
      border-radius: 12px;
      box-shadow: 0 8px 24px rgba(0, 0, 0, 0.16);
      border: 1px solid var(--divider-color);
      padding: 6px 0;
      overflow: auto;
      box-sizing: border-box;
    }
    .item {
      width: 100%;
      border: none;
      background: transparent;
      display: flex;
      align-items: center;
      gap: 10px;
      padding: 10px 14px;
      font-size: 14px;
      color: var(--primary-text-color);
      cursor: pointer;
      text-align: left;
    }
    .item:hover {
      background: rgba(0, 0, 0, 0.04);
    }
    .item.destructive {
      color: var(--error-color, #b91c1c);
    }
    .divider {
      height: 1px;
      background: var(--divider-color);
      margin: 4px 0;
    }
    .icon {
      width: 18px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      opacity: 0.8;
      --mdc-icon-size: 18px;
    }
  `;Qs([u({attribute:!1})],Kr.prototype,"items",2);Qs([g()],Kr.prototype,"_open",2);Qs([g()],Kr.prototype,"_menuStyle",2);Kr=Qs([x("ha-action-menu")],Kr);var R_=Object.defineProperty,F_=Object.getOwnPropertyDescriptor,Pt=(e,t,i,r)=>{for(var o=r>1?void 0:r?F_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&R_(t,i,o),o};let Be=class extends y{constructor(){super(...arguments),this.sectionIndex=0,this.cardIndex=0,this.noEdit=!1,this.hiddenOverlay=!1,this.compact=!1,this.inactive=!1,this._hover=!1,this._focused=!1,this._menuOpen=!1,this._touchStarted=!1,this._documentClicked=e=>{this._hover=e.composedPath().includes(this),document.removeEventListener("click",this._documentClicked)}}connectedCallback(){super.connectedCallback(),this._unsubMenuOpen=hc(e=>{this.contains(e)||(this._hover=!1,this._focused=!1,this._menuOpen=!1)}),this.addEventListener("focus",()=>{this._focused=!0}),this.addEventListener("blur",()=>{this._focused=!1}),this.addEventListener("touchstart",()=>{this._touchStarted=!0,this._hover=!0,document.addEventListener("click",this._documentClicked)}),this.addEventListener("touchend",()=>{setTimeout(()=>{this._touchStarted=!1},10)}),this.addEventListener("mouseenter",()=>{this._touchStarted||(this._hover=!0)}),this.addEventListener("mouseleave",()=>{this._menuOpen||(this._hover=!1)}),this.addEventListener("click",()=>{this._hover=!0,document.addEventListener("click",this._documentClicked)})}disconnectedCallback(){var e;document.removeEventListener("click",this._documentClicked),(e=this._unsubMenuOpen)==null||e.call(this),super.disconnectedCallback()}updated(e){e.has("inactive")&&this.inactive&&(this._hover=!1,this._focused=!1,this._menuOpen=!1,this.toggleAttribute("menu-open",!1))}_path(){return{sectionIndex:this.sectionIndex,cardIndex:this.cardIndex}}_menuItems(){const e=[];return this.noEdit||e.push({value:"edit",label:"编辑",icon:"mdi:pencil"}),e.push({value:"duplicate",label:"创建副本",icon:"mdi:plus-circle-multiple-outline"}),e.push({value:"copy",label:"复制",icon:"mdi:content-copy"}),e.push({value:"cut",label:"剪切",icon:"mdi:content-cut"}),e.length&&e.push({value:"divider",label:"",divider:!0}),e.push({value:"delete",label:"删除",icon:"mdi:delete",destructive:!0}),e}_onMenuAction(e){const t=this._path();switch(e.detail.value){case"edit":Y(this,"ll-edit-card",t);break;case"duplicate":Y(this,"ll-duplicate-card",t);break;case"copy":Y(this,"ll-copy-card",t);break;case"cut":Y(this,"ll-copy-card",t),Y(this,"ll-delete-card",{...t,silent:!0});break;case"delete":Y(this,"ll-delete-card",{...t,silent:!1});break}}_onMenuOpened(){this._menuOpen=!0,this.toggleAttribute("menu-open",!0),this._hover=!0}_onMenuClosed(){this._menuOpen=!1,this.toggleAttribute("menu-open",!1)}_editOverlay(e){this.noEdit||(e.preventDefault(),e.stopPropagation(),Y(this,"ll-edit-card",this._path()))}render(){if(this.inactive)return h`
        <div class="card-wrapper">
          <slot></slot>
        </div>
      `;const e=this.hiddenOverlay||this.compact,t=(this._hover||this._focused||this._menuOpen)&&!e,i=this._hover||this._focused||this._menuOpen,r=this._hover||this._focused||this._menuOpen;return h`
      <div class="card-wrapper ${r?"elevated":""}">
        <slot></slot>
        <div
          class=${T({"card-overlay":!0,visible:t,compact:e,"menu-visible":i})}
        >
          ${e||this.noEdit?m:h`
                <button type="button" class="control" @click=${this._editOverlay} title="编辑">
                  <div class="control-overlay"></div>
                  <ha-icon class="control-icon" icon="mdi:pencil"></ha-icon>
                </button>
              `}
          ${this.noEdit&&!e?h`
                <div class="control drag-hint" title="拖动以调整位置">
                  <div class="control-overlay"></div>
                  <ha-icon class="control-icon" icon="mdi:cursor-move"></ha-icon>
                </div>
              `:m}
          <ha-action-menu
            class="more ${i?"":"hidden"}"
            .items=${this._menuItems()}
            @action=${this._onMenuAction}
            @menu-opened=${this._onMenuOpened}
            @menu-closed=${this._onMenuClosed}
          >
            <ha-icon slot="trigger" icon="mdi:dots-vertical"></ha-icon>
          </ha-action-menu>
        </div>
      </div>
    `}};Be.styles=w`
    :host {
      display: block;
      height: 100%;
      position: relative;
      z-index: 0;
    }
    :host(:hover),
    :host(:focus-within) {
      z-index: 2;
    }
    :host([menu-open]) {
      z-index: 5;
    }
    .card-wrapper {
      position: relative;
      height: 100%;
      z-index: 0;
    }
    .card-wrapper.elevated {
      z-index: 2;
    }
    .card-overlay {
      position: absolute;
      inset: 0;
      opacity: 0;
      pointer-events: none;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: opacity 180ms ease-in-out;
      z-index: 2;
    }
    .card-overlay.visible {
      opacity: 1;
      pointer-events: auto;
    }
    .card-overlay.compact {
      align-items: flex-start;
      justify-content: flex-end;
    }
    .card-overlay.compact.visible,
    .card-overlay.compact.menu-visible {
      opacity: 1;
      pointer-events: none;
    }
    .control {
      outline: none;
      cursor: pointer;
      position: absolute;
      inset: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      border: none;
      background: transparent;
      border-radius: var(--ha-card-border-radius, 12px);
      padding: 0;
    }
    .control-overlay {
      position: absolute;
      inset: 0;
      opacity: 0.8;
      background-color: var(--primary-background-color);
      border: 1px solid var(--divider-color);
      border-radius: var(--ha-card-border-radius, 12px);
    }
    .control-icon {
      position: relative;
      z-index: 1;
      color: var(--primary-text-color);
      border-radius: var(--ha-border-radius-circle, 50%);
      padding: 8px;
      background-color: var(--secondary-background-color);
      --mdc-icon-size: 20px;
    }
    .more {
      position: absolute;
      right: -6px;
      top: -6px;
      z-index: 3;
      pointer-events: auto;
      --ha-action-menu-trigger-background: var(--secondary-background-color);
      --ha-action-menu-trigger-hover-background: color-mix(
        in srgb,
        var(--secondary-background-color) 88%,
        #000000
      );
      --ha-action-menu-trigger-size: 32px;
      --ha-action-menu-trigger-radius: 50%;
    }
    .more.hidden {
      opacity: 0;
      pointer-events: none;
    }
    .card-overlay.compact .more {
      right: 0;
      top: 0;
    }
  `;Pt([u({type:Number})],Be.prototype,"sectionIndex",2);Pt([u({type:Number})],Be.prototype,"cardIndex",2);Pt([u({type:Boolean,attribute:"no-edit"})],Be.prototype,"noEdit",2);Pt([u({type:Boolean,attribute:"hidden-overlay"})],Be.prototype,"hiddenOverlay",2);Pt([u({type:Boolean})],Be.prototype,"compact",2);Pt([u({type:Boolean,reflect:!0})],Be.prototype,"inactive",2);Pt([g()],Be.prototype,"_hover",2);Pt([g()],Be.prototype,"_focused",2);Pt([g()],Be.prototype,"_menuOpen",2);Be=Pt([x("hui-card-edit-mode")],Be);var B_=Object.defineProperty,U_=Object.getOwnPropertyDescriptor,At=(e,t,i,r)=>{for(var o=r>1?void 0:r?U_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&B_(t,i,o),o};let Ue=class extends y{constructor(){super(...arguments),this.sectionIndex=0,this.cardIndex=0,this.editMode=!1,this.preview=!1,this._statesRevision=0,this._configKey="",this._entityUnsubs=[],this._lastType=""}_refreshCard(){var t;const e=this._card;(t=e==null?void 0:e.requestUpdate)==null||t.call(e),this.requestUpdate()}connectedCallback(){super.connectedCallback()}disconnectedCallback(){this._clearEntitySubscriptions(),super.disconnectedCallback()}_clearEntitySubscriptions(){this._entityUnsubs.forEach(e=>e()),this._entityUnsubs=[]}_syncEntitySubscriptions(){this._clearEntitySubscriptions();const e=this.flow;if(!e||!this._card)return;const t=Gp(this.config);if(t.length===0){this._entityUnsubs.push(e.subscribeStates(()=>{this._statesRevision=e.statesRevision,this._refreshCard()}));return}t.forEach(i=>{this._entityUnsubs.push(e.subscribeEntity(i,()=>{this._statesRevision=e.statesRevision,this._refreshCard()}))})}async updated(e){if(e.has("config")&&this.config){const t=JSON.stringify(this.config);if(t!==this._configKey)if(this._configKey=t,this._card&&this._lastType===this.config.type)this._card.setConfig(this.config);else{this._lastType=this.config.type;const i=await Ks(this.config);i.flow=this.flow,i.preview=this.preview,this._card=i}}this._card&&(e.has("flow")||e.has("config")||e.has("preview"))&&(this._card.flow=this.flow,this._card.preview=this.preview),(e.has("flow")||e.has("config"))&&this._syncEntitySubscriptions(),this._card&&this._applyGridSize()}_applyGridSize(){const e=I_(this._card,this.config),t=Wp(e),{columnSize:i,rowSize:r,fullWidth:o,autoHeight:n}=M_(t);this.style.setProperty("--column-size",String(i)),this.style.setProperty("--row-size",String(r)),this.classList.toggle("full-width",o),this.classList.toggle("fit-rows",!n),this.toggleAttribute("auto-height",n)}render(){var r,o,n,s,a,l;if(!this._card)return m;const e=((r=this.config)==null?void 0:r.type)==="heading",t=((o=this.config)==null?void 0:o.type)==="line3d"||((n=this.config)==null?void 0:n.type)==="scatter3d"||((s=this.config)==null?void 0:s.type)==="line2d"||((a=this.config)==null?void 0:a.type)==="scatter2d"||((l=this.config)==null?void 0:l.type)==="bar";this._statesRevision;const i=h`
      <hui-card-edit-mode
        .inactive=${!this.editMode}
        .sectionIndex=${this.sectionIndex}
        .cardIndex=${this.cardIndex}
        .noEdit=${!1}
        .compact=${e||t}
        .hiddenOverlay=${e}
      >
        ${this._card}
      </hui-card-edit-mode>
    `;return h`${i}`}};Ue.styles=w`
    :host {
      display: block;
      position: relative;
      grid-row: span var(--row-size, 1);
      grid-column: span min(var(--column-size, 1), var(--grid-column-count, 12));
      min-width: 0;
      border-radius: var(--ha-card-border-radius, 12px);
    }
    :host(.full-width) {
      grid-column: 1 / -1;
    }
    :host(.fit-rows) {
      height: calc(
        (var(--row-size, 1) * (var(--row-height, 56px) + var(--row-gap, 8px))) -
          var(--row-gap, 8px)
      );
    }
    :host([auto-height]) {
      height: auto;
      min-height: auto;
    }
    :host(.dragging) {
      opacity: 0.5;
    }
  `;At([cr({context:vi,subscribe:!0}),u({attribute:!1})],Ue.prototype,"flow",2);At([u({attribute:!1})],Ue.prototype,"config",2);At([u({type:Number})],Ue.prototype,"sectionIndex",2);At([u({type:Number})],Ue.prototype,"cardIndex",2);At([u({attribute:!1})],Ue.prototype,"sortableData",2);At([u({type:Boolean})],Ue.prototype,"editMode",2);At([u({type:Boolean})],Ue.prototype,"preview",2);At([g()],Ue.prototype,"_card",2);At([g()],Ue.prototype,"_statesRevision",2);Ue=At([x("hui-card-container")],Ue);var j_=Object.defineProperty,q_=Object.getOwnPropertyDescriptor,Yp=(e,t,i,r)=>{for(var o=r>1?void 0:r?q_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&j_(t,i,o),o};let ks=class extends y{updated(){const e=$h(this.background);if(!e){this.style.removeProperty("--section-background-color"),this.style.removeProperty("--section-background-opacity");return}const t=e.color&&e.color!=="default"?e.color:null;this.style.setProperty("--section-background-color",t?Nt(t):"var(--ha-section-background-color, var(--secondary-background-color))");const i=e.opacity!==void 0?e.opacity:Zn;this.style.setProperty("--section-background-opacity",`${i}%`)}render(){return m}};ks.styles=w`
    :host {
      position: absolute;
      inset: 0;
      border-radius: inherit;
      background-color: var(--section-background-color);
      opacity: var(--section-background-opacity, 50%);
      z-index: 0;
      pointer-events: none;
    }
  `;Yp([u({attribute:!1})],ks.prototype,"background",2);ks=Yp([x("flow-section-background")],ks);var G_=Object.defineProperty,X_=Object.getOwnPropertyDescriptor,Tt=(e,t,i,r)=>{for(var o=r>1?void 0:r?X_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&G_(t,i,o),o};const W_={delay:100,delayOnTouchOnly:!0,direction:"vertical",invertedSwapThreshold:.7,group:"card"};let je=class extends y{constructor(){super(...arguments),this.viewIndex=0,this.sectionIndex=0,this.editMode=!1,this.preview=!1,this.layoutRevision=0,this._dragging=!1,this._cardConfigKeys=new WeakMap}_getCardKey(e){return this._cardConfigKeys.has(e)||this._cardConfigKeys.set(e,Math.random().toString(36)),this._cardConfigKeys.get(e)}_addCard(){this.dispatchEvent(new CustomEvent("add-card",{bubbles:!0,composed:!0,detail:{section:this.config}}))}_notifyLayoutChange(){this.dispatchEvent(new CustomEvent("layout-will-change",{bubbles:!0,composed:!0})),this.dispatchEvent(new CustomEvent("layout-changed",{bubbles:!0,composed:!0})),this.requestUpdate()}_cardMoved(e){if(e.stopPropagation(),!this.view)return;const{oldIndex:t,newIndex:i}=e.detail;t!==i&&(_d(this.view,this.sectionIndex,t,this.sectionIndex,i),this._notifyLayoutChange())}_cardAdded(e){if(e.stopPropagation(),!this.view)return;const t=yv(e.detail.data);t&&(t.sectionIndex===this.sectionIndex&&t.viewIndex===this.viewIndex||(_d(this.view,t.sectionIndex,t.cardIndex,this.sectionIndex,e.detail.index),this._notifyLayoutChange()))}_cardRemoved(e){e.stopPropagation(),e.detail.index}_dragStart(){this._dragging=!0}_dragEnd(){this._dragging=!1}render(){if(!this.config)return m;const e=this.config.cards,t=$h(this.config.background)!==void 0,i=h`
      <div class="card-grid ${this.editMode?"edit-mode":""} ${this._dragging?"dragging":""}">
        ${Tp(e,(r,o)=>`${this.layoutRevision}:${o}:${this._getCardKey(r)}`,(r,o)=>h`
            <hui-card-container
              .sortableData=${[this.viewIndex,this.sectionIndex,o]}
              .config=${r}
              .sectionIndex=${this.sectionIndex}
              .cardIndex=${o}
              .editMode=${this.editMode}
              .preview=${this.preview}
            ></hui-card-container>
          `)}
        ${this.editMode?h`
              <button type="button" class="add-card" @click=${this._addCard}>
                <span class="add-plus">+</span>
              </button>
            `:m}
      </div>
    `;return h`
      <div class="section-root ${this.editMode?"edit-mode":""}">
        <div
          class="section-container ${t?"has-background":""}"
        >
          ${t?h`
                <flow-section-background
                  .background=${this.config.background}
                ></flow-section-background>
              `:m}
          <div class="section-wrapper">
            <ha-sortable
              no-style
              invert-swap
              .disabled=${!this.editMode}
              .rollback=${!1}
              draggable-selector="hui-card-container"
              filter=".add-card,button,.trigger,.menu,.chart-interactive,canvas"
              .options=${W_}
              @item-moved=${this._cardMoved}
              @item-added=${this._cardAdded}
              @item-removed=${this._cardRemoved}
              @drag-start=${this._dragStart}
              @drag-end=${this._dragEnd}
            >
              ${i}
            </ha-sortable>
          </div>
        </div>
      </div>
    `}};je.styles=w`
    :host {
      --base-column-count: 12;
      --row-gap: var(--ha-section-grid-row-gap, 8px);
      --column-gap: var(--ha-section-grid-column-gap, 8px);
      --row-height: var(--ha-section-grid-row-height, 56px);
      display: block;
    }

    .section-root {
      display: flex;
      flex-direction: column;
    }

    .section-container {
      position: relative;
      border-radius: var(--ha-section-border-radius, 16px);
    }

    .section-container.has-background {
      padding: var(--ha-space-2, 8px);
    }

    .section-wrapper {
      position: relative;
      z-index: 1;
      border-radius: inherit;
    }

    .card-grid {
      --grid-column-count: calc(var(--base-column-count) * var(--column-span, 1));
      display: grid;
      grid-template-columns: repeat(var(--grid-column-count), minmax(0, 1fr));
      grid-auto-rows: auto;
      grid-auto-flow: row dense;
      row-gap: var(--row-gap);
      column-gap: var(--column-gap);
      align-items: stretch;
    }

    .card-grid.dragging hui-card-container {
      cursor: grabbing;
    }

    .card-grid.edit-mode hui-card-container:not([auto-height]) {
      min-height: calc((var(--row-height) - var(--row-gap)) / 2);
    }

    .add-card {
      grid-column: span 3;
      grid-row: span 1;
      height: var(--row-height);
      border: 2px dashed var(--primary-color);
      border-radius: var(--ha-card-border-radius, 12px);
      background: transparent;
      color: var(--primary-color);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 0;
      order: 1;
    }

    .add-card:hover {
      background: rgba(3, 169, 244, 0.04);
    }

    .add-plus {
      font-size: 28px;
      line-height: 1;
      font-weight: 300;
    }
  `;Tt([cr({context:vi,subscribe:!0}),u({attribute:!1})],je.prototype,"flow",2);Tt([u({attribute:!1})],je.prototype,"config",2);Tt([u({attribute:!1})],je.prototype,"view",2);Tt([u({type:Number})],je.prototype,"viewIndex",2);Tt([u({type:Number})],je.prototype,"sectionIndex",2);Tt([u({type:Boolean})],je.prototype,"editMode",2);Tt([u({type:Boolean})],je.prototype,"preview",2);Tt([u({type:Number})],je.prototype,"layoutRevision",2);Tt([g()],je.prototype,"_dragging",2);je=Tt([x("flow-section")],je);var Y_=Object.defineProperty,Z_=Object.getOwnPropertyDescriptor,uc=(e,t,i,r)=>{for(var o=r>1?void 0:r?Z_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Y_(t,i,o),o};let tn=class extends y{constructor(){super(...arguments),this.sectionIndex=0,this.inactive=!1}connectedCallback(){super.connectedCallback(),this._unsubMenuOpen=hc(e=>{this.contains(e)||this.toggleAttribute("menu-open",!1)})}disconnectedCallback(){var e;(e=this._unsubMenuOpen)==null||e.call(this),super.disconnectedCallback()}_path(){return{sectionIndex:this.sectionIndex}}_menuItems(){return[{value:"edit",label:"编辑",icon:"mdi:pencil"},{value:"duplicate",label:"复制",icon:"mdi:content-copy"},{value:"divider",label:"",divider:!0},{value:"delete",label:"删除",icon:"mdi:delete",destructive:!0}]}_onMenuAction(e){const t=this._path();switch(e.detail.value){case"edit":Y(this,"ll-edit-section",t);break;case"duplicate":Y(this,"ll-duplicate-section",t);break;case"delete":Y(this,"ll-delete-section",t);break}}render(){return h`
      <div class="section-header">
        <div class="section-actions">
          <span class="section-handle handle" title="拖动以调整分区位置">≡</span>
          <ha-action-menu
            .items=${this._menuItems()}
            @action=${this._onMenuAction}
            @menu-opened=${()=>{this.toggleAttribute("menu-open",!0)}}
            @menu-closed=${()=>{this.toggleAttribute("menu-open",!1)}}
          ></ha-action-menu>
        </div>
      </div>
      <div class="section-wrapper">
        <slot></slot>
      </div>
    `}};tn.styles=w`
    :host {
      display: block;
      position: relative;
      z-index: 10;
    }
    :host([menu-open]) {
      z-index: 20;
    }
    :host([inactive]) {
      z-index: auto;
    }
    :host([inactive]) .section-header {
      display: none;
    }
    :host([inactive]) .section-wrapper {
      padding: 0;
      border: none;
      min-height: 0;
    }
    .section-header {
      position: relative;
      height: 34px;
      display: flex;
      flex-direction: column;
      justify-content: flex-end;
    }
    .section-actions {
      position: absolute;
      height: 36px;
      bottom: -2px;
      right: 0;
      inset-inline-end: 0;
      inset-inline-start: initial;
      display: flex;
      align-items: center;
      justify-content: center;
      border-radius: var(--ha-section-border-radius, 16px);
      border-bottom-left-radius: 0;
      border-bottom-right-radius: 0;
      background-color: var(--secondary-background-color);
      color: var(--primary-text-color);
      --ha-icon-button-size: 36px;
      --mdc-icon-size: 20px;
      z-index: 2;
    }
    .handle {
      cursor: grab;
      padding: 8px;
      user-select: none;
      font-size: 18px;
      line-height: 1;
    }
    .handle:active {
      cursor: grabbing;
    }
    ha-action-menu {
      display: inline-flex;
      --ha-action-menu-trigger-background: transparent;
      --ha-action-menu-trigger-hover-background: rgba(0, 0, 0, 0.04);
      --ha-action-menu-trigger-size: 36px;
      --ha-action-menu-trigger-radius: 0 var(--ha-section-border-radius, 16px) 0 0;
    }
    .section-wrapper {
      padding: var(--ha-space-2, 8px);
      border-radius: var(--ha-section-border-radius, 16px);
      border-start-end-radius: 0;
      border: 2px dashed var(--divider-color);
      min-height: var(--row-height, var(--ha-view-sections-row-height, 56px));
      box-sizing: border-box;
    }
  `;uc([u({type:Number})],tn.prototype,"sectionIndex",2);uc([u({type:Boolean,reflect:!0})],tn.prototype,"inactive",2);tn=uc([x("hui-section-edit-mode")],tn);var K_=Object.defineProperty,Q_=Object.getOwnPropertyDescriptor,wi=(e,t,i,r)=>{for(var o=r>1?void 0:r?Q_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&K_(t,i,o),o};const J_={delay:100,delayOnTouchOnly:!0,animation:150};let _t=class extends y{constructor(){super(...arguments),this.viewIndex=0,this.editMode=!1,this.layoutRevision=0,this._maxColumns=1,this._dragging=!1,this._sectionConfigKeys=new WeakMap}_getSectionKey(e){return this._sectionConfigKeys.has(e)||this._sectionConfigKeys.set(e,Math.random().toString(36)),this._sectionConfigKeys.get(e)}connectedCallback(){super.connectedCallback(),this._resizeObserver=new ResizeObserver(e=>{var i;const t=((i=e[0])==null?void 0:i.contentRect.width)??0;t&&this._updateMaxColumns(t)}),this._resizeObserver.observe(this)}disconnectedCallback(){var e;(e=this._resizeObserver)==null||e.disconnect(),super.disconnectedCallback()}_updateMaxColumns(e){var c;const t=getComputedStyle(this),i=Vn(t.getPropertyValue("--column-min-width"))||320,r=Vn(t.getPropertyValue("--column-gap"))||32,o=this.renderRoot.querySelector(".wrapper"),n=o?getComputedStyle(o):null,s=n?Vn(n.paddingLeft)+Vn(n.paddingRight):r*2,a=((c=this.config)==null?void 0:c.max_columns)??Mp,l=Kv(e,{minColumnWidth:i,columnGap:r,horizontalPadding:s,configMaxColumns:a});l!==this._maxColumns&&(this._maxColumns=l)}_addSection(){this.dispatchEvent(new CustomEvent("add-section",{bubbles:!0,composed:!0}))}_sectionMoved(e){if(e.stopPropagation(),!this.config)return;const{oldIndex:t,newIndex:i}=e.detail;t!==i&&(this.dispatchEvent(new CustomEvent("layout-will-change",{bubbles:!0,composed:!0})),xv(this.config,t,i),this.dispatchEvent(new CustomEvent("layout-changed",{bubbles:!0,composed:!0})),this.requestUpdate())}_dragStart(){this._dragging=!0}_dragEnd(){this._dragging=!1}render(){if(!this.config)return m;const e=this.config.sections,t=Zv(e),i=Qv({sectionColumnSpanSum:t,maxColumns:this._maxColumns,editMode:this.editMode}),r=!!this.config.dense_section_placement,o=h`
      <div class="container ${this._dragging?"dragging":""} ${r?"dense":""}">
        ${Tp(e,n=>this._getSectionKey(n),(n,s)=>{const a=Jv(n.column_span,i),l=n.row_span??1;return h`
            <div
              class="section-slot ${this.editMode?"edit-mode":""}"
              style=${`--column-span: ${a}; --row-span: ${l};`}
            >
              <hui-section-edit-mode .sectionIndex=${s} .inactive=${!this.editMode}>
                <flow-section
                  style=${`--column-span: ${a};`}
                  .view=${this.config}
                  .viewIndex=${this.viewIndex}
                  .config=${n}
                  .sectionIndex=${s}
                  .editMode=${this.editMode}
                  .layoutRevision=${this.layoutRevision}
                ></flow-section>
              </hui-section-edit-mode>
            </div>
          `})}
        ${this.editMode?h`
              <button type="button" class="create-section" @click=${this._addSection}>
                <span class="create-icon">▦+</span>
                <span>拖放卡片到此处以创建新分区</span>
              </button>
            `:m}
      </div>
    `;return h`
      <div
        class="wrapper"
        style=${`--column-count: ${this._maxColumns}; --content-column-count: ${i};`}
      >
        <ha-sortable
          no-style
          .disabled=${!this.editMode}
          draggable-selector=".section-slot"
          handle-selector=".section-handle"
          filter=".create-section,button,.trigger,.menu"
          .options=${J_}
          @item-moved=${this._sectionMoved}
          @drag-start=${this._dragStart}
          @drag-end=${this._dragEnd}
        >
          ${o}
        </ha-sortable>
      </div>
    `}};_t.styles=w`
    :host {
      --row-height: var(--ha-view-sections-row-height, 56px);
      --row-gap: var(--ha-view-sections-row-gap, 24px);
      --column-gap: var(--ha-view-sections-column-gap, 32px);
      --column-max-width: var(--ha-view-sections-column-max-width, 500px);
      --column-min-width: var(--ha-view-sections-column-min-width, 320px);
      display: block;
      flex: 1;
    }

    .wrapper {
      padding: 0 var(--column-gap);
      box-sizing: content-box;
      margin: var(--ha-view-sections-extra-top-margin, 24px) auto 0;
      max-width: calc(
        var(--column-count) * var(--column-max-width) +
          (var(--column-count) - 1) * var(--column-gap)
      );
    }

    .container {
      display: grid;
      grid-template-columns: repeat(var(--content-column-count), minmax(0, 1fr));
      grid-auto-flow: row;
      gap: var(--row-gap) var(--column-gap);
      padding: var(--row-gap) 0;
      align-items: start;
    }

    .container.dense {
      grid-auto-flow: row dense;
    }

    .section-slot {
      grid-column: span var(--column-span, 1);
      grid-row: span var(--row-span, 1);
      min-width: 0;
      display: flex;
      flex-direction: column;
      position: relative;
    }

    .container.dragging .section-slot {
      cursor: grabbing;
    }

    .create-section {
      grid-column: span 1;
      min-height: calc(var(--row-height) + 2 * var(--row-gap));
      border: 2px dashed var(--primary-color);
      border-radius: var(--ha-section-border-radius, 16px);
      background: transparent;
      color: var(--primary-text-color);
      cursor: pointer;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 8px;
      padding: 16px;
      font-size: 14px;
      line-height: 1.4;
      text-align: center;
    }

    .create-section:hover {
      background: rgba(3, 169, 244, 0.04);
    }

    .create-icon {
      font-size: 28px;
      line-height: 1;
      color: var(--primary-color);
      font-weight: 300;
    }

    @media (max-width: 600px) {
      .container {
        grid-template-columns: 1fr;
      }
      .section-slot {
        grid-column: 1 / -1;
      }
    }
  `;wi([cr({context:vi,subscribe:!0}),u({attribute:!1})],_t.prototype,"flow",2);wi([u({attribute:!1})],_t.prototype,"config",2);wi([u({type:Number})],_t.prototype,"viewIndex",2);wi([u({type:Boolean})],_t.prototype,"editMode",2);wi([u({type:Number})],_t.prototype,"layoutRevision",2);wi([g()],_t.prototype,"_maxColumns",2);wi([g()],_t.prototype,"_dragging",2);_t=wi([x("flow-view")],_t);var e0=Object.defineProperty,t0=Object.getOwnPropertyDescriptor,Js=(e,t,i,r)=>{for(var o=r>1?void 0:r?t0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&e0(t,i,o),o};let Qr=class extends y{constructor(){super(...arguments),this.views=[],this.activeIndex=0,this.editMode=!1}_select(e){e!==this.activeIndex&&this.dispatchEvent(new CustomEvent("view-selected",{bubbles:!0,composed:!0,detail:{index:e,path:rl(this.views[e],e)}}))}_add(){this.dispatchEvent(new CustomEvent("view-add",{bubbles:!0,composed:!0}))}_edit(e,t){t.stopPropagation(),this.dispatchEvent(new CustomEvent("view-edit",{bubbles:!0,composed:!0,detail:{index:e}}))}_delete(e,t){t.stopPropagation(),this.dispatchEvent(new CustomEvent("view-delete",{bubbles:!0,composed:!0,detail:{index:e}}))}render(){return this.views.length?h`
      <div class="tabs" role="tablist">
        <div class="scroll">
          ${this.views.map((e,t)=>{const i=t===this.activeIndex,r=e.title||e.path||`视图 ${t+1}`,o=e.icon||"mdi:view-dashboard";return h`
              <button
                type="button"
                role="tab"
                class="tab ${i?"active":""}"
                aria-selected=${i?"true":"false"}
                title=${r}
                data-path=${rl(e,t)}
                @click=${()=>this._select(t)}
                @dblclick=${n=>{this.editMode&&this._edit(t,n)}}
              >
                <ha-icon class="tab-icon" .icon=${o}></ha-icon>
                <span class="tab-label">${r}</span>
                ${this.editMode&&i?h`
                      <span
                        class="tab-edit"
                        title="编辑视图"
                        role="button"
                        tabindex="0"
                        @click=${n=>this._edit(t,n)}
                        @keydown=${n=>{(n.key==="Enter"||n.key===" ")&&(n.preventDefault(),this._edit(t,n))}}
                      >
                        <ha-svg-icon .path=${Ah}></ha-svg-icon>
                      </span>
                    `:m}
                ${this.editMode&&this.views.length>1?h`
                      <span
                        class="tab-delete"
                        title="删除视图"
                        role="button"
                        tabindex="0"
                        @click=${n=>this._delete(t,n)}
                        @keydown=${n=>{(n.key==="Enter"||n.key===" ")&&(n.preventDefault(),this._delete(t,n))}}
                      >
                        ×
                      </span>
                    `:m}
              </button>
            `})}
        </div>
        ${this.editMode?h`
              <button
                type="button"
                class="add"
                title="添加视图"
                aria-label="添加视图"
                @click=${this._add}
              >
                <ha-svg-icon .path=${Cf}></ha-svg-icon>
              </button>
            `:m}
      </div>
    `:m}};Qr.styles=w`
    :host {
      display: block;
      min-width: 0;
    }

    .tabs {
      display: flex;
      align-items: stretch;
      gap: 4px;
      min-width: 0;
      height: 100%;
    }

    .scroll {
      display: flex;
      align-items: stretch;
      gap: 2px;
      min-width: 0;
      overflow-x: auto;
      scrollbar-width: none;
    }

    .scroll::-webkit-scrollbar {
      display: none;
    }

    .tab {
      position: relative;
      display: inline-flex;
      align-items: center;
      gap: 8px;
      height: 100%;
      min-height: 40px;
      padding: 0 14px;
      border: none;
      border-bottom: 2px solid transparent;
      background: transparent;
      color: var(--app-header-text-color, var(--primary-text-color));
      opacity: 0.72;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
      font-weight: 500;
      white-space: nowrap;
      box-sizing: border-box;
      flex-shrink: 0;
    }

    .tab:hover {
      opacity: 1;
      background: rgba(0, 0, 0, 0.04);
    }

    .tab.active {
      opacity: 1;
      border-bottom-color: var(
        --ha-tab-indicator-color,
        var(--app-header-edit-text-color, var(--primary-color, #03a9f4))
      );
    }

    .tab-icon {
      --mdc-icon-size: 20px;
      width: 20px;
      height: 20px;
      flex-shrink: 0;
    }

    .tab-label {
      max-width: 160px;
      overflow: hidden;
      text-overflow: ellipsis;
    }

    .tab-edit,
    .tab-delete {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 22px;
      height: 22px;
      margin-left: 2px;
      border-radius: 50%;
      opacity: 0.65;
      flex-shrink: 0;
    }

    .tab-edit ha-svg-icon {
      width: 16px;
      height: 16px;
    }

    .tab-delete {
      font-size: 16px;
      line-height: 1;
    }

    .tab-edit:hover,
    .tab-delete:hover {
      opacity: 1;
      background: rgba(0, 0, 0, 0.12);
    }

    .tab-delete:hover {
      color: var(--error-color, #db4437);
    }

    .add {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 40px;
      height: 40px;
      margin: auto 0;
      border: none;
      border-radius: 50%;
      background: transparent;
      color: inherit;
      cursor: pointer;
      flex-shrink: 0;
      opacity: 0.85;
    }

    .add:hover {
      opacity: 1;
      background: rgba(0, 0, 0, 0.08);
    }

    .add ha-svg-icon {
      width: 22px;
      height: 22px;
    }
  `;Js([u({attribute:!1})],Qr.prototype,"views",2);Js([u({type:Number})],Qr.prototype,"activeIndex",2);Js([u({type:Boolean})],Qr.prototype,"editMode",2);Qr=Js([x("flow-view-tabs")],Qr);function mr(e){let t=!1;return{onBackdropPointerDown:n=>{t=n.target===n.currentTarget},onBackdropPointerUp:n=>{t&&n.target===n.currentTarget&&e(),t=!1},onBackdropPointerCancel:()=>{t=!1}}}function Un(e,t=0){if(e==null||e==="")return t;const i=Number(e);return Number.isFinite(i)?i:t}function Vo(e){if(Array.isArray(e)&&e.length>=1){const t=Un(e[0]),i=Un(e[1]);return e.slice(0,2).some(r=>r!=null&&r!==""&&Number.isFinite(Number(r)))?[t,i]:null}if(e&&typeof e=="object"){const t=e,i=t.default??t.values;return Array.isArray(i)&&i.length>=1?Vo(i):t.x!==void 0||t.X!==void 0||t.y!==void 0||t.Y!==void 0?[Un(t.x??t.X),Un(t.y??t.Y)]:null}return null}function Cs(e){if(Array.isArray(e))return e.map(Vo).filter(t=>!!t);if(typeof e=="string"){const t=e.trim();if(!t)return[];try{return Cs(JSON.parse(t))}catch{return[]}}return[]}function $r(e,t){const i=Number.isFinite(t)&&t>0?Math.floor(t):5e3;return e.length>i?e.slice(-i):e}function Zp(e,t,i){if(t==null)return e;let r=t;if(typeof t=="string"){const o=t.trim();if(!o)return e;if(o.toLowerCase()==="clear")return[];try{r=JSON.parse(o)}catch{const n=o.split(/[,;\s]+/).filter(a=>a.length>0),s=Vo(n);return s?$r([...e,s],i):e}}if(r==="clear")return[];if(Array.isArray(r)){if(r.length>=1&&r.length<=3&&r.every(o=>typeof o=="number"||typeof o=="string")){const o=Vo(r);return o?$r([...e,o],i):e}return $r(Cs(r),i)}if(r&&typeof r=="object"){const o=r;if(o.clear===!0)return[];if(Array.isArray(o.points)){const s=Cs(o.points);return o.append===!0||o.mode==="append"?$r([...e,...s],i):$r(s,i)}if(o.value!==void 0&&o.x===void 0&&o.X===void 0)return Zp(e,o.value,i);const n=Vo(o);if(n)return $r([...e,n],i)}return e}function i0(e=80){const t=[];for(let i=0;i<e;i+=1){const r=i/4;t.push([r,Math.sin(r/3)*5+Math.cos(r/7)*2])}return t}function r0(e=60){const t=[];for(let i=0;i<e;i+=1)t.push([(Math.random()-.5)*12,(Math.random()-.5)*12]);return t}class Ow{constructor(){this.points=[],this._lastChanged=0,this._entity="",this._maxPoints=5e3}bind(t,i,r,o){if(this.dispose(),this._onChange=o,this._maxPoints=Number.isFinite(r)&&r>0?Math.floor(r):5e3,this._entity=i??"",!t||!i)return;let n=!0;this._unsub=t.subscribeEntity(i,s=>{s&&(this._consumeState(s,n),n=!1)})}dispose(){var t;(t=this._unsub)==null||t.call(this),this._unsub=void 0,this._lastChanged=0,this._entity="",this.points=[]}resetForEntity(t){t!==this._entity&&(this.points=[],this._lastChanged=0)}_consumeState(t,i){var o,n;if(t.last_changed<=this._lastChanged)return;i&&((o=t.attributes)!=null&&o.points)&&(this.points=Cs(t.attributes.points));const r=t.state;r!=null&&r!==""&&(this.points=Zp(this.points,r,this._maxPoints)),this._lastChanged=t.last_changed,(n=this._onChange)==null||n.call(this)}}const He={bg_color:"",max_points:5e3,line_width:2,line_color:"#03a9f4",symbol_size:8,point_color:"#03a9f4",bar_color:"#03a9f4",x_label:"X",y_label:"Y",show_axes:!0,show_grid:!0};function kr(e,t=0){if(e==null||e==="")return t;const i=Number(e);return Number.isFinite(i)?i:t}function No(e){if(Array.isArray(e)&&e.length>=1){const t=kr(e[0]),i=kr(e[1]),r=kr(e[2]);return e.slice(0,3).some(o=>o!=null&&o!==""&&Number.isFinite(Number(o)))?[t,i,r]:null}if(e&&typeof e=="object"){const t=e,i=t.default??t.values;return Array.isArray(i)&&i.length>=1?No(i):t.x!==void 0||t.X!==void 0||t.y!==void 0||t.Y!==void 0||t.z!==void 0||t.Z!==void 0?[kr(t.x??t.X),kr(t.y??t.Y),kr(t.z??t.Z)]:null}return null}function Ss(e){if(Array.isArray(e))return e.map(No).filter(t=>!!t);if(typeof e=="string"){const t=e.trim();if(!t)return[];try{return Ss(JSON.parse(t))}catch{return[]}}return[]}function Cr(e,t){const i=Number.isFinite(t)&&t>0?Math.floor(t):5e3;return e.length>i?e.slice(-i):e}function Kp(e,t,i){if(t==null)return e;let r=t;if(typeof t=="string"){const o=t.trim();if(!o)return e;if(o.toLowerCase()==="clear")return[];try{r=JSON.parse(o)}catch{const n=o.split(/[,;\s]+/).filter(a=>a.length>0),s=No(n);return s?Cr([...e,s],i):e}}if(r==="clear")return[];if(Array.isArray(r)){if(r.length>=1&&r.length<=4&&r.every(o=>typeof o=="number"||typeof o=="string")){const o=No(r);return o?Cr([...e,o],i):e}return Cr(Ss(r),i)}if(r&&typeof r=="object"){const o=r;if(o.clear===!0)return[];if(Array.isArray(o.points)){const s=Ss(o.points);return o.append===!0||o.mode==="append"?Cr([...e,...s],i):Cr(s,i)}if(o.value!==void 0&&o.x===void 0&&o.X===void 0)return Kp(e,o.value,i);const n=No(o);if(n)return Cr([...e,n],i)}return e}function o0(e=120){const t=[];for(let i=0;i<e;i+=1){const r=i/12;t.push([Math.cos(r)*6,Math.sin(r)*6,r*.8])}return t}function n0(e=80){const t=[];for(let i=0;i<e;i+=1)t.push([(Math.random()-.5)*12,(Math.random()-.5)*12,(Math.random()-.5)*12]);return t}class Lw{constructor(){this.points=[],this._lastChanged=0,this._entity="",this._maxPoints=5e3}bind(t,i,r,o){if(this.dispose(),this._onChange=o,this._maxPoints=Number.isFinite(r)&&r>0?Math.floor(r):5e3,this._entity=i??"",!t||!i)return;let n=!0;this._unsub=t.subscribeEntity(i,s=>{s&&(this._consumeState(s,n),n=!1)})}dispose(){var t;(t=this._unsub)==null||t.call(this),this._unsub=void 0,this._lastChanged=0,this._entity="",this.points=[]}resetForEntity(t){t!==this._entity&&(this.points=[],this._lastChanged=0)}_consumeState(t,i){var o,n;if(t.last_changed<=this._lastChanged)return;i&&((o=t.attributes)!=null&&o.points)&&(this.points=Ss(t.attributes.points));const r=t.state;r!=null&&r!==""&&(this.points=Kp(this.points,r,this._maxPoints)),this._lastChanged=t.last_changed,(n=this._onChange)==null||n.call(this)}}const Ke={bg_color:"",max_points:5e3,line_width:3,line_color:"#03a9f4",symbol_size:8,point_color:"#03a9f4",x_label:"X",y_label:"Y",z_label:"Z",show_axes:!0,auto_rotate:!1},s0="data:image/svg+xml,"+encodeURIComponent(`<svg xmlns="http://www.w3.org/2000/svg" width="96" height="56" viewBox="0 0 96 56" fill="none">
      <rect x="10" y="12" width="76" height="32" rx="6" fill="#fff" stroke="#e0e0e0"/>
      <circle cx="28" cy="28" r="8" fill="#cfd8dc"/>
      <rect x="44" y="22" width="30" height="5" rx="2.5" fill="#cfd8dc"/>
      <rect x="44" y="31" width="20" height="5" rx="2.5" fill="#cfd8dc"/>
    </svg>`),a0="data:image/svg+xml,"+encodeURIComponent(`<svg xmlns="http://www.w3.org/2000/svg" width="96" height="56" viewBox="0 0 96 56" fill="none">
      <rect x="22" y="6" width="52" height="44" rx="6" fill="#fff" stroke="#e0e0e0"/>
      <circle cx="48" cy="20" r="7" fill="#cfd8dc"/>
      <rect x="34" y="32" width="28" height="4" rx="2" fill="#cfd8dc"/>
      <rect x="38" y="39" width="20" height="4" rx="2" fill="#cfd8dc"/>
    </svg>`),l0={name:"",label:"",type:"grid",gridVariant:"appearance",schema:[{name:"icon",label:"图标",type:"icon",optional:!0,compact:!0},{name:"color",label:"颜色",type:"color",compact:!0}]},ne={name:"content_layout",label:"内容布局",type:"select_box",options:[{value:"horizontal",label:"水平",image:s0},{value:"vertical",label:"垂直",image:a0}]},Es={name:"content",label:"内容",type:"section",schema:[{name:"name",label:"名称",type:"text",optional:!0,helper:"留空则使用动作默认名称"},l0]},c0={name:"entity",label:"动作",type:"entity"};function xe(e=[]){const t=[...Es.schema??[],...e];return[c0,{...Es,schema:t}]}const d0=[{x:10,y:88},{x:28,y:18},{x:72,y:18},{x:90,y:88}],h0=[{type:"entity",entity:"/demo/path_progress",name:"轨迹点 1",icon:"mdi:robot-industrial",color:"accent",show_icon:!0,show_name:!1,show_state:!1}],La=[{name:"bg_color",label:"背景色",type:"color",optional:!0}],Da=[{name:"x_label",label:"X 轴标签",type:"text",optional:!0,placeholder:He.x_label},{name:"y_label",label:"Y 轴标签",type:"text",optional:!0,placeholder:He.y_label},{name:"show_axes",label:"显示坐标轴",type:"boolean"},{name:"show_grid",label:"显示网格线",type:"boolean"}],zd=[{name:"max_points",label:"最大点数",type:"number",helper:"超出后丢弃最旧的数据点"}],Vd=[{name:"bg_color",label:"背景色",type:"color",optional:!0}],Nd=[{name:"x_label",label:"X 轴标签",type:"text",optional:!0,placeholder:Ke.x_label},{name:"y_label",label:"Y 轴标签",type:"text",optional:!0,placeholder:Ke.y_label},{name:"z_label",label:"Z 轴标签",type:"text",optional:!0,placeholder:Ke.z_label},{name:"show_axes",label:"显示坐标轴",type:"boolean"},{name:"auto_rotate",label:"自动旋转",type:"boolean"}],Rd=[{name:"max_points",label:"最大点数",type:"number",helper:"超出后丢弃最旧的数据点"}],p0={heading:[{name:"heading_style",label:"样式",type:"select",options:[{value:"title",label:"Title"},{value:"subtitle",label:"副标题"}]},{name:"heading",label:"标题文字",type:"text",placeholder:"客厅"},{name:"icon",label:"图标",type:"icon",placeholder:"mdi:sofa",optional:!0},{name:"badges",label:"徽章",type:"badges"}],tile:[...xe([{name:"hide_state",label:"隐藏状态",type:"boolean"},ne])],sensor:[...xe([{name:"hide_state",label:"隐藏状态",type:"boolean"},ne]),{name:"unit",label:"单位",type:"text",optional:!0,placeholder:"°C"}],label:[...xe([{name:"hide_state",label:"隐藏状态",type:"boolean"},ne])],switch:[...xe([ne])],trigger:[...xe([ne])],mic:[{name:"entity",label:"端口地址",type:"entity",helper:"绑定 Web Mic 节点的 /port（如 /dataflow/<id>/port）"},{...Es,schema:[...Es.schema??[],ne]},{name:"host",label:"主机",type:"text",optional:!0,placeholder:"留空则用当前页面主机名",helper:"Web Mic 节点所在机器；通常留空"},{name:"port",label:"备用端口",type:"number",optional:!0,helper:"端口地址无值时使用（调试用）"}],slider:[...xe([ne]),{name:"range",label:"范围",type:"grid",schema:[{name:"min",label:"最小值",type:"number"},{name:"max",label:"最大值",type:"number"},{name:"step",label:"步进",type:"number"}]}],climate:[...xe([ne]),{name:"range",label:"范围",type:"grid",schema:[{name:"min",label:"最小值",type:"number"},{name:"max",label:"最大值",type:"number"},{name:"step",label:"步进",type:"number"}]},{name:"unit",label:"单位",type:"text",optional:!0,placeholder:"°C"}],gain:[...xe([ne]),{name:"range",label:"范围 (dB)",type:"grid",schema:[{name:"min",label:"最小值",type:"number"},{name:"max",label:"最大值",type:"number"},{name:"step",label:"步进",type:"number"}]}],rgba:[...xe([ne])],hsv:[...xe([ne])],color:[...xe([ne])],"multi-fader":[...xe([ne,{name:"hide_state",label:"隐藏状态",type:"boolean"}]),{name:"count",label:"维度数量",type:"number",helper:"推杆个数（1–16），输出对应长度的数组"},{name:"labels",label:"通道标签",type:"text",optional:!0,placeholder:"0,1,2,3",helper:"用逗号分隔，数量不足时从 0 自动补序号"},{name:"orientation",label:"推杆方向",type:"select",options:[{value:"vertical",label:"竖直"},{value:"horizontal",label:"水平"}]},{name:"range",label:"范围",type:"grid",schema:[{name:"min",label:"最小值",type:"number"},{name:"max",label:"最大值",type:"number"},{name:"step",label:"步进",type:"number"}]}],"xy-pad":[{name:"entity",label:"动作（向量）",type:"entity",optional:!0,helper:"写入 [x, y] 数组；若同时填写 X/Y 动作则优先使用分动作模式"},{name:"axes",label:"分动作（可选）",type:"grid",schema:[{name:"entity_x",label:"X 动作",type:"entity",optional:!0},{name:"entity_y",label:"Y 动作",type:"entity",optional:!0}]},{name:"content",label:"内容",type:"section",schema:[{name:"name",label:"名称",type:"text",optional:!0,helper:"留空则使用地址默认名称"},{name:"",label:"",type:"grid",gridVariant:"appearance",schema:[{name:"icon",label:"图标",type:"icon",optional:!0,compact:!0},{name:"color",label:"颜色",type:"color",compact:!0}]},ne,{name:"show_grid",label:"显示十字线",type:"boolean"},{name:"invert_y",label:"Y 轴向上为负",type:"boolean",helper:"开启后触控板顶部对应 y_min"},{name:"snap_center",label:"松手回中",type:"boolean"}]},{name:"range",label:"范围",type:"grid",schema:[{name:"x_min",label:"X 最小",type:"number"},{name:"x_max",label:"X 最大",type:"number"},{name:"y_min",label:"Y 最小",type:"number"},{name:"y_max",label:"Y 最大",type:"number"},{name:"step",label:"步进",type:"number"}]}],cover:[...xe([ne]),{name:"position_count",label:"档位数量",type:"number",helper:"默认生成 0、1、2… 档位；留空自定义 positions 时优先使用 positions"}],media:[...xe([ne]),{name:"play_value",label:"播放指令",type:"text",optional:!0,placeholder:"true",helper:"写入地址的值，支持 true / false / 数字 / 字符串"},{name:"stop_value",label:"停止指令",type:"text",optional:!0,placeholder:"false"}],clock:[{name:"title",label:"标题",type:"text",optional:!0},{name:"clock_style",label:"样式",type:"select",options:[{value:"digital",label:"数字"},{value:"analog",label:"模拟"}]},{name:"clock_size",label:"尺寸",type:"select",options:[{value:"small",label:"小"},{value:"medium",label:"中"},{value:"large",label:"大"}]},{name:"time_format",label:"时间格式",type:"select",options:[{value:"auto",label:"跟随系统"},{value:"24",label:"24 小时"},{value:"12",label:"12 小时"}],optional:!0},{name:"show_seconds",label:"显示秒",type:"boolean"},{name:"no_background",label:"无背景",type:"boolean"},{name:"time_zone",label:"时区",type:"text",optional:!0,placeholder:"Asia/Shanghai",helper:"留空则使用浏览器时区"},{name:"analog_border",label:"表盘边框",type:"boolean",visible:{field:"clock_style",operator:"eq",value:"analog"}},{name:"analog_face_style",label:"表盘数字",type:"select",options:[{value:"markers",label:"刻度"},{value:"numbers",label:"阿拉伯数字"},{value:"roman",label:"罗马数字"}],visible:{field:"clock_style",operator:"eq",value:"analog"}},{name:"analog_ticks",label:"刻度",type:"select",options:[{value:"hour",label:"小时"},{value:"quarter",label:"一刻钟"},{value:"minute",label:"分钟"},{value:"none",label:"无"}],visible:{field:"clock_style",operator:"eq",value:"analog"}}],markdown:[{name:"style",label:"样式",type:"select",options:[{value:"card",label:"卡片"},{value:"text-only",label:"纯文本"}]},{name:"title",label:"标题",type:"text",optional:!0,visible:{field:"style",operator:"not_eq",value:"text-only"}},{name:"content",label:"内容",type:"textarea",helper:"支持 Markdown 语法"}],link:[{name:"name",label:"名称",type:"text",placeholder:"文档"},{name:"url",label:"链接地址",type:"text",placeholder:"http://127.0.0.1:8992/"},{name:"icon",label:"图标",type:"icon",optional:!0,placeholder:"mdi:link-variant"},{name:"new_tab",label:"新标签页打开",type:"boolean"}],"picture-elements":[{name:"title",label:"标题",type:"text",optional:!0,placeholder:"户型图"},{name:"image",label:"底图",type:"image"},{name:"elements",label:"元素",type:"picture_elements"}],"path-badge":[{name:"title",label:"标题",type:"text",optional:!0,placeholder:""},{name:"image",label:"底图",type:"image",optional:!0},{name:"points",label:"轨迹绘制",type:"path_points",helper:"点击底图新增轨迹点，拖动点调整位置；值变化时只会移动徽章，轨迹本身保持不动"},{name:"badges",label:"轨迹徽章",type:"path_badges"},{name:"",label:"",type:"grid",schema:[{name:"track_color",label:"轨迹颜色",type:"color"},{name:"track_width",label:"轨迹线宽",type:"number"}]},{name:"",label:"",type:"grid",schema:[{name:"progress_min",label:"范围最小值",type:"number"},{name:"progress_max",label:"范围最大值",type:"number"}]},{name:"",label:"",type:"grid",schema:[{name:"badge_size",label:"徽章尺寸",type:"number"},{name:"caption",label:"说明文字",type:"text",optional:!0,placeholder:"可选"}]}],line3d:[{name:"entity",label:"动作",type:"entity"},{name:"name",label:"名称",type:"text",optional:!0},...Rd,{name:"line_width",label:"线宽",type:"number"},{name:"line_color",label:"线条颜色",type:"color",optional:!0},...Vd,...Nd],scatter3d:[{name:"entity",label:"动作",type:"entity"},{name:"name",label:"名称",type:"text",optional:!0},...Rd,{name:"symbol_size",label:"点大小",type:"number"},{name:"point_color",label:"点颜色",type:"color",optional:!0},...Vd,...Nd],line2d:[{name:"entity",label:"动作",type:"entity"},{name:"name",label:"名称",type:"text",optional:!0},...zd,{name:"line_width",label:"线宽",type:"number"},{name:"line_color",label:"线条颜色",type:"color",optional:!0},...La,...Da],scatter2d:[{name:"entity",label:"动作",type:"entity"},{name:"name",label:"名称",type:"text",optional:!0},...zd,{name:"symbol_size",label:"点大小",type:"number"},{name:"point_color",label:"点颜色",type:"color",optional:!0},...La,...Da],bar:[{name:"name",label:"名称",type:"text",optional:!0},{name:"entities",label:"柱",type:"bar_entities"},{name:"bar_color",label:"默认柱颜色",type:"color",optional:!0,helper:"未单独设色的柱使用此颜色"},...La,...Da],status:[{name:"name",label:"名称",type:"text",optional:!0,placeholder:"Flow 连接"},{name:"",label:"",type:"grid",gridVariant:"appearance",schema:[{name:"icon",label:"图标",type:"icon",optional:!0,compact:!0},{name:"color",label:"正常色",type:"color",compact:!0}]},ne,{name:"list_columns",label:"状态列数",type:"select",options:[{value:"1",label:"单列"},{value:"2",label:"双列"}]},{name:"entities",label:"状态项",type:"status_entities"}]};function u0(e){return p0[e==="button"?"trigger":e==="color"?"rgba":e]??[{name:"entity",label:"动作",type:"entity",optional:!0},{name:"name",label:"名称",type:"text",optional:!0}]}function f0(e){const t=e.type,i=!!e.vertical,r={...e.grid_options??{}};if(t==="tile"||t==="trigger"||t==="sensor"||t==="label"){const o=i?2:1;e.grid_options={...r,columns:r.columns??6,rows:o,min_rows:o,min_columns:i?3:6};return}if(t==="mic"){const o=i?2:1;e.grid_options={...r,columns:r.columns??6,rows:o,min_rows:o,min_columns:i?3:6};return}if(t==="multi-fader"){const o=Math.max(1,Math.min(16,Math.round(Number(e.count)||4))),n=e.orientation==="horizontal"?"horizontal":"vertical",s=n==="vertical"?4:Math.max(2,Math.min(8,1+Math.ceil(o/2)));e.grid_options={...r,columns:r.columns??(n==="vertical"?Math.min(12,Math.max(6,o*2)):6),rows:s,min_rows:n==="vertical"?4:s,min_columns:i?3:6};return}if(t==="xy-pad"||t==="rgba"||t==="hsv"||t==="color"){const o=t==="xy-pad"?i?4:3:5;e.grid_options={...r,columns:r.columns??6,rows:o,min_rows:o,min_columns:i?3:6};return}if(t==="slider"||t==="switch"||t==="climate"||t==="cover"||t==="gain"||t==="media"){const o=i?3:2;e.grid_options={...r,columns:r.columns??6,rows:o,min_rows:o,min_columns:i?3:6};return}t==="status"&&(e.grid_options={...r,columns:r.columns??(i?3:6),rows:"auto",min_rows:2,min_columns:i?3:6})}function g0(e,t){const i={...e};i.type==="button"&&(i.type="trigger"),i.type==="color"&&(i.type="rgba"),i.content_layout!==void 0&&(i.vertical=i.content_layout==="vertical",delete i.content_layout),t!=null&&t.layoutChanged&&(i.type==="tile"||i.type==="trigger"||i.type==="sensor"||i.type==="label"||i.type==="mic"||i.type==="slider"||i.type==="switch"||i.type==="climate"||i.type==="cover"||i.type==="gain"||i.type==="media"||i.type==="rgba"||i.type==="hsv"||i.type==="color"||i.type==="xy-pad"||i.type==="multi-fader"||i.type==="status")&&f0(i),i.style!==void 0&&(i.style==="text-only"?i.text_only=!0:delete i.text_only,delete i.style),i.time_format==="auto"&&delete i.time_format,i.color||delete i.color;const r=i.type==="markdown"?["range",""]:["content","range",""];for(const o of r)o in i&&delete i[o];return i.type==="status"&&(Number(i.list_columns)===2?i.list_columns=2:delete i.list_columns),i.type==="path-badge"&&(String(i.caption??"").trim()||delete i.caption),i}function m0(e){const t={...e};if("vertical"in t?t.content_layout=t.vertical?"vertical":"horizontal":(t.type==="tile"||t.type==="trigger"||t.type==="mic"||t.type==="slider"||t.type==="switch"||t.type==="climate"||t.type==="cover"||t.type==="gain"||t.type==="media"||t.type==="status")&&(t.content_layout="horizontal"),t.text_only?t.style="text-only":t.type==="markdown"&&(t.style="card"),t.type==="clock"&&!t.time_format&&(t.time_format="auto"),t.type==="climate"&&(t.min===void 0&&(t.min=0),t.max===void 0&&(t.max=100),t.step===void 0&&(t.step=1)),t.type==="gain"&&(t.min===void 0&&(t.min=-60),t.max===void 0&&(t.max=12),t.step===void 0&&(t.step=1)),t.type==="cover"&&t.position_count===void 0&&(t.position_count=4),t.type==="clock"&&(t.clock_style||(t.clock_style="digital"),t.clock_size||(t.clock_size="small")),t.type==="clock"&&t.analog_border===void 0&&t.clock_style==="analog"&&(t.analog_border=!0),t.type==="clock"&&!t.analog_face_style&&t.clock_style==="analog"&&(t.analog_face_style="markers"),t.type==="heading"&&(t.heading_style||(t.heading_style="title"),t.heading||(t.heading="新建部件"),t.badges||(t.badges=[])),!t.elements&&t.type==="picture-elements"&&(t.elements=[]),t.type==="path-badge"&&(t.image||(t.image=""),t.progress_min===void 0&&(t.progress_min=0),t.progress_max===void 0&&(t.progress_max=1),(!Array.isArray(t.points)||t.points.length===0)&&!String(t.path??"").trim()&&(t.points=d0.map(i=>({...i}))),!Array.isArray(t.badges)||t.badges.length===0)){const i=String(t.entity??"").trim();i?t.badges=[{type:"entity",entity:i,name:String(t.name??"").trim()||void 0,icon:String(t.icon??"").trim()||void 0,color:String(t.badge_color??"").trim()||void 0,show_icon:!0,show_name:!!t.show_state,show_state:!!t.show_state,attribute:String(t.attribute??"").trim()||void 0}]:t.badges=h0.map(r=>({...r}))}return t.type==="line3d"&&(t.max_points===void 0&&(t.max_points=Ke.max_points),t.line_width===void 0&&(t.line_width=Ke.line_width),t.show_axes===void 0&&(t.show_axes=Ke.show_axes),t.auto_rotate===void 0&&(t.auto_rotate=Ke.auto_rotate)),t.type==="scatter3d"&&(t.max_points===void 0&&(t.max_points=Ke.max_points),t.symbol_size===void 0&&(t.symbol_size=Ke.symbol_size),t.show_axes===void 0&&(t.show_axes=Ke.show_axes),t.auto_rotate===void 0&&(t.auto_rotate=Ke.auto_rotate)),t.type==="line2d"&&(t.max_points===void 0&&(t.max_points=He.max_points),t.line_width===void 0&&(t.line_width=He.line_width),t.show_axes===void 0&&(t.show_axes=He.show_axes),t.show_grid===void 0&&(t.show_grid=He.show_grid)),t.type==="scatter2d"&&(t.max_points===void 0&&(t.max_points=He.max_points),t.symbol_size===void 0&&(t.symbol_size=He.symbol_size),t.show_axes===void 0&&(t.show_axes=He.show_axes),t.show_grid===void 0&&(t.show_grid=He.show_grid)),t.type==="bar"&&(t.entities||(t.entities=[]),t.show_axes===void 0&&(t.show_axes=He.show_axes),t.show_grid===void 0&&(t.show_grid=He.show_grid)),t.type==="status"&&(t.entities||(t.entities=[]),t.icon||(t.icon="mdi:lan-connect"),t.list_columns=Number(t.list_columns)===2?"2":"1"),t.type==="xy-pad"&&(t.show_grid===void 0&&(t.show_grid=j.show_grid),t.snap_center===void 0&&(t.snap_center=j.snap_center),t.invert_y===void 0&&(t.invert_y=j.invert_y),t.x_min===void 0&&(t.x_min=j.x_min),t.x_max===void 0&&(t.x_max=j.x_max),t.y_min===void 0&&(t.y_min=j.y_min),t.y_max===void 0&&(t.y_max=j.y_max),t.step===void 0&&(t.step=j.step)),t.type==="link"&&(t.new_tab===void 0&&(t.new_tab=!0),t.url||(t.url="http://127.0.0.1:8992/")),t}var b0=Object.defineProperty,v0=Object.getOwnPropertyDescriptor,ea=(e,t,i,r)=>{for(var o=r>1?void 0:r?v0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&b0(t,i,o),o};let Jr=class extends y{constructor(){super(...arguments),this.header="",this.expanded=!0,this._open=!0}willUpdate(e){e.has("expanded")&&(this._open=this.expanded)}_toggle(){this._open=!this._open}render(){return h`
      <div class="panel">
        <button type="button" class="header" @click=${this._toggle}>
          <span class="chevron ${T({open:this._open})}">›</span>
          <span class="title">${this.header}</span>
        </button>
        ${this._open?h`<div class="body"><slot></slot></div>`:""}
      </div>
    `}};Jr.styles=w`
    .panel {
      border: 1px solid var(--divider-color);
      border-radius: 12px;
      margin-bottom: 12px;
      background: var(--card-background-color, #fff);
    }
    .header {
      width: 100%;
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 12px 14px;
      border: none;
      background: transparent;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
      font-weight: 600;
      color: var(--primary-text-color);
      text-align: left;
    }
    .chevron {
      display: inline-block;
      transition: transform 180ms ease;
      color: var(--secondary-text-color);
      font-size: 18px;
      line-height: 1;
    }
    .chevron.open {
      transform: rotate(90deg);
    }
    .body {
      padding: 0 14px 14px;
    }
  `;ea([u()],Jr.prototype,"header",2);ea([u({type:Boolean,reflect:!0})],Jr.prototype,"expanded",2);ea([g()],Jr.prototype,"_open",2);Jr=ea([x("ha-expandable-panel")],Jr);function pe(e){var o,n,s,a;const t=((o=e.name)==null?void 0:o.trim())||((n=e.suggestedName)==null?void 0:n.trim());if(t)return t.replace(/^\d+:\s*/,"");const i=(s=e.nodeName)==null?void 0:s.trim();if(i)return i;const r=(a=e.nodeType)==null?void 0:a.trim();return r||e.entity}function _0(e){switch(e){case"read":return"只读";case"write":return"只写";default:return"读写"}}function Fd(e,t){const i=t.trim();if(i)return e.find(r=>r.entity===i)}function Bd(e,t){const i=t.trim().toLowerCase();return i?e.filter(r=>pe(r).toLowerCase().includes(i)):[...e]}async function y0(e,t){const i=e.trim();if(!i)return;const r=(t==null?void 0:t.trim())||i;try{const o=await fetch("/api/actions",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({entity:i,name:r})});if(!o.ok)return;const n=await o.json();return Jp(),n.item}catch{return}}async function Qp(){try{const e=await fetch("/api/actions");if(!e.ok)return[];const t=await e.json();return Array.isArray(t.items)?t.items:[]}catch{return[]}}async function x0(e){try{const t=await fetch("/api/actions/remove",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({entity:e})});if(!t.ok)return!1;const r=!!(await t.json()).ok;return r&&Jp(),r}catch{return!1}}function Jp(){document.dispatchEvent(new CustomEvent("flow-actions-changed"))}var w0=Object.defineProperty,$0=Object.getOwnPropertyDescriptor,Xe=(e,t,i,r)=>{for(var o=r>1?void 0:r?$0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&w0(t,i,o),o};let ke=class extends y{constructor(){super(...arguments),this.label="动作",this.value="",this.placeholder="搜索动作名",this.optional=!1,this.compact=!1,this._actions=[],this._open=!1,this._menuStyle={},this._focused=!1,this._query="",this._editedQuery=!1,this._suppressNextBlur=!1,this._onActionsChanged=()=>{this._loadActions()},this._onDocumentClick=e=>{if(!this._open)return;e.composedPath().includes(this)||this._close()},this._repositionMenu=()=>{this._open&&this._updateMenuPosition()}}connectedCallback(){super.connectedCallback(),document.addEventListener("click",this._onDocumentClick),document.addEventListener("flow-actions-changed",this._onActionsChanged),window.addEventListener("resize",this._repositionMenu),this._loadActions()}disconnectedCallback(){document.removeEventListener("click",this._onDocumentClick),document.removeEventListener("flow-actions-changed",this._onActionsChanged),window.removeEventListener("resize",this._repositionMenu),this._detachScrollListener(),super.disconnectedCallback()}async _loadActions(){const e=await Qp();this._actions=e;const t=e.map(i=>i.entity).filter(Boolean);t.length&&this.flow&&this.flow.connection.trackAddresses(t)}_inputText(){if(this._focused)return this._query;const e=Fd(this._actions,this.value);return e?pe(e):""}_actionOptions(){return[...Bd(this._actions,this._focused?this._query:"")].sort((t,i)=>t.used!==i.used?t.used?1:-1:pe(t).localeCompare(pe(i),"zh-CN")).map(t=>({action:t,label:pe(t),hint:t.entity}))}_emitSelection(e,t){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:e,actionName:t}}))}_attachScrollListener(){this._detachScrollListener();let e=this.parentElement;for(;e;){const t=getComputedStyle(e);if(/(auto|scroll)/.test(t.overflowY)){this._scrollParent=e,e.addEventListener("scroll",this._repositionMenu,{passive:!0});return}e=e.parentElement}}_detachScrollListener(){var e;(e=this._scrollParent)==null||e.removeEventListener("scroll",this._repositionMenu),this._scrollParent=void 0}_updateMenuPosition(){var i;const e=(i=this.shadowRoot)==null?void 0:i.querySelector(".input-wrap");if(!e)return;const t=pc(e);this._menuStyle={position:"fixed",top:`${t.top}px`,left:`${t.left}px`,width:`${t.width}px`,maxHeight:`${t.maxHeight}px`,zIndex:"500"}}_openMenu(){this._open||(this._open=!0,this._attachScrollListener(),this.requestUpdate(),requestAnimationFrame(()=>this._updateMenuPosition()))}_close(){this._open&&(this._open=!1,this._detachScrollListener())}_onInput(e){this._editedQuery=!0,this._query=e.target.value,this._openMenu(),requestAnimationFrame(()=>this._updateMenuPosition())}_onFocus(){this._loadActions(),this._focused=!0,this._editedQuery=!1;const e=Fd(this._actions,this.value);this._query=e?pe(e):"",this._openMenu()}_onBlur(){window.setTimeout(()=>{if(this._suppressNextBlur){this._suppressNextBlur=!1,this._focused=!1,this._query="",this._close();return}this._focused=!1;const e=this._query.trim();if(!e){this.optional&&this._editedQuery&&this._emitSelection(""),this._query="",this._close();return}const t=Bd(this._actions,e).find(i=>pe(i).toLowerCase()===e.toLowerCase());t&&t.entity!==this.value&&this._emitSelection(t.entity,pe(t)),this._query="",this._close()},0)}_selectAction(e){this._suppressNextBlur=!0,this._focused=!1,this._editedQuery=!1,this._query="",this._close(),this._emitSelection(e.entity,pe(e))}_renderOption(e){return h`
      <button
        type="button"
        class="option"
        @mousedown=${t=>t.preventDefault()}
        @click=${()=>this._selectAction(e.action)}
      >
        <span class="option-value">${e.label}</span>
        ${e.hint?h`<span class="option-hint">${e.hint}</span>`:m}
      </button>
    `}render(){const e=this._actionOptions(),t=e.length>0;return h`
      <label class="field ${this.compact?"compact":""}">
        ${this.compact?m:h`
              <span class="label">
                ${this.label}
                ${this.optional?h`<span class="optional">（可选）</span>`:""}
              </span>
            `}
        <div class="input-wrap">
          <input
            type="text"
            .value=${this._inputText()}
            placeholder=${this.placeholder}
            @input=${this._onInput}
            @focus=${this._onFocus}
            @blur=${this._onBlur}
          />
        </div>
      </label>
      ${this._open&&t?h`
            <div class="menu" style=${P(this._menuStyle)}>
              <div class="section">
                <div class="section-title">动作库</div>
                ${e.map(i=>this._renderOption(i))}
              </div>
            </div>
          `:this._open?h`
              <div class="menu" style=${P(this._menuStyle)}>
                <div class="section empty-hint">
                  <p>未找到匹配动作，请先在侧边栏「动作库」中添加</p>
                </div>
              </div>
            `:m}
    `}};ke.styles=w`
    :host {
      display: block;
      position: relative;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
    }
    .field.compact {
      gap: 0;
      margin-bottom: 0;
    }
    .field.compact input {
      height: 36px;
      padding: 0 10px;
      border-radius: 8px;
    }
    .label {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .optional {
      font-weight: 400;
      opacity: 0.8;
    }
    .input-wrap {
      display: block;
    }
    input {
      width: 100%;
      box-sizing: border-box;
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .menu {
      overflow-y: auto;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      background: #fff;
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.12);
    }
    .section-title {
      padding: 8px 12px 4px;
      font-size: 11px;
      font-weight: 600;
      letter-spacing: 0.02em;
      text-transform: uppercase;
      color: var(--secondary-text-color);
    }
    .empty-hint p {
      margin: 0;
      padding: 10px 12px;
      font-size: 12px;
      color: var(--secondary-text-color);
    }
    .option {
      display: flex;
      flex-direction: column;
      align-items: flex-start;
      gap: 2px;
      width: 100%;
      padding: 8px 12px;
      border: none;
      background: transparent;
      cursor: pointer;
      font: inherit;
      text-align: left;
      color: var(--primary-text-color);
    }
    .option:hover,
    .option:focus-visible {
      background: rgba(var(--rgb-primary-color, 3, 169, 244), 0.08);
    }
    .option-value {
      font-size: 14px;
      line-height: 1.3;
      word-break: break-all;
    }
    .option-hint {
      font-size: 12px;
      color: var(--primary-color);
    }
  `;Xe([cr({context:vi,subscribe:!0}),u({attribute:!1})],ke.prototype,"flow",2);Xe([u()],ke.prototype,"label",2);Xe([u()],ke.prototype,"value",2);Xe([u()],ke.prototype,"placeholder",2);Xe([u({type:Boolean})],ke.prototype,"optional",2);Xe([u({type:Boolean,reflect:!0})],ke.prototype,"compact",2);Xe([g()],ke.prototype,"_actions",2);Xe([g()],ke.prototype,"_open",2);Xe([g()],ke.prototype,"_menuStyle",2);Xe([g()],ke.prototype,"_focused",2);Xe([g()],ke.prototype,"_query",2);ke=Xe([x("ha-entity-picker")],ke);var k0=Object.defineProperty,C0=Object.getOwnPropertyDescriptor,lo=(e,t,i,r)=>{for(var o=r>1?void 0:r?C0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&k0(t,i,o),o};let pi=class extends y{constructor(){super(...arguments),this.label="",this.hasValue=!1,this.disabled=!1,this.opened=!1,this.compact=!1}render(){const e=this.hasValue&&this.label?h`<span class="overline">${this.label}</span>`:m,t=this.hasValue?h`<span class="headline"><slot name="value"></slot></span>`:h`<span class="headline placeholder">${this.label}</span>`;return h`
      <button
        type="button"
        class="field ${this.compact?"compact":""}"
        ?disabled=${this.disabled}
        part="button"
      >
        <span class="leading"><slot name="leading"></slot></span>
        <span class="text">
          ${e}
          ${t}
        </span>
        <span class="trailing" aria-hidden="true">
          <svg viewBox="0 0 24 24" class="chevron">
            <path d="M7 10l5 5 5-5z"></path>
          </svg>
        </span>
      </button>
    `}};pi.styles=w`
    :host {
      display: block;
      min-width: 0;
    }
    .field {
      width: 100%;
      min-height: 56px;
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 0 8px 0 12px;
      border: none;
      border-radius: 4px 4px 0 0;
      background: var(--ha-color-form-background, #f5f5f5);
      cursor: pointer;
      font: inherit;
      text-align: left;
      color: var(--primary-text-color);
      position: relative;
      box-sizing: border-box;
    }
    .field:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
    .field.compact {
      min-height: 36px;
      padding: 0 6px 0 8px;
      border-radius: 8px;
    }
    .field.compact .text {
      padding: 4px 0;
    }
    .field.compact .overline {
      display: none;
    }
    .field.compact .headline {
      font-size: 13px;
    }
    .field.compact .trailing {
      flex-basis: 24px;
    }
    .field.compact .chevron {
      width: 18px;
      height: 18px;
    }
    .field::after {
      content: "";
      position: absolute;
      left: 0;
      right: 0;
      bottom: 0;
      height: 1px;
      background: var(--ha-color-border-neutral-loud, var(--divider-color, #ccc));
      transition:
        height 180ms ease,
        background-color 180ms ease;
    }
    :host([opened]) .field::after {
      height: 2px;
      background: var(--primary-color);
    }
    .leading {
      flex: 0 0 auto;
      display: flex;
      align-items: center;
      justify-content: center;
      width: 24px;
      height: 24px;
    }
    .text {
      flex: 1;
      min-width: 0;
      display: flex;
      flex-direction: column;
      justify-content: center;
      gap: 2px;
      padding: 8px 0;
    }
    .overline {
      font-size: 12px;
      line-height: 1.2;
      color: var(--secondary-text-color);
    }
    .headline {
      font-size: 14px;
      line-height: 1.3;
      font-weight: 400;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .headline.placeholder {
      color: var(--secondary-text-color);
    }
    .trailing {
      flex: 0 0 32px;
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--secondary-text-color);
    }
    .chevron {
      width: 20px;
      height: 20px;
      fill: currentColor;
    }
  `;lo([u()],pi.prototype,"label",2);lo([u({type:Boolean})],pi.prototype,"hasValue",2);lo([u({type:Boolean})],pi.prototype,"disabled",2);lo([u({type:Boolean,reflect:!0})],pi.prototype,"opened",2);lo([u({type:Boolean,reflect:!0})],pi.prototype,"compact",2);pi=lo([x("ha-picker-field")],pi);var S0=Object.defineProperty,E0=Object.getOwnPropertyDescriptor,Mt=(e,t,i,r)=>{for(var o=r>1?void 0:r?E0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&S0(t,i,o),o};let qe=class extends y{constructor(){super(...arguments),this.label="图标",this.value="",this.placeholder="mdi:home",this.compact=!1,this._open=!1,this._filter="",this._results=[],this._registryReady=!1,this._menuStyle={},this._onDocumentClick=e=>{if(!this._open)return;e.composedPath().includes(this)||this._close()},this._repositionMenu=()=>{this._open&&this._updateMenuPosition()}}connectedCallback(){super.connectedCallback(),document.addEventListener("click",this._onDocumentClick),window.addEventListener("resize",this._repositionMenu),this._results=On(""),this._unloadRegistryListener=Th(()=>{this._registryReady=!0,this._results=On(this._filter),this.requestUpdate()}),Na()}disconnectedCallback(){var e;document.removeEventListener("click",this._onDocumentClick),window.removeEventListener("resize",this._repositionMenu),this._detachScrollListener(),(e=this._unloadRegistryListener)==null||e.call(this),super.disconnectedCallback()}_attachScrollListener(){this._detachScrollListener();let e=this.parentElement;for(;e;){const t=getComputedStyle(e);if(/(auto|scroll)/.test(t.overflowY)){this._scrollParent=e,e.addEventListener("scroll",this._repositionMenu,{passive:!0});return}e=e.parentElement}}_detachScrollListener(){var e;(e=this._scrollParent)==null||e.removeEventListener("scroll",this._repositionMenu),this._scrollParent=void 0}_updateMenuPosition(){var i;const e=(i=this.shadowRoot)==null?void 0:i.querySelector("ha-picker-field");if(!e)return;const t=pc(e);this._menuStyle={position:"fixed",top:`${t.top}px`,left:`${t.left}px`,width:`${t.width}px`,maxHeight:`${t.maxHeight}px`,zIndex:"500"}}_toggleOpen(e){if(e.stopPropagation(),this._open){this._close();return}Na().then(()=>{this._filter="",this._results=On(""),this._open=!0,this._attachScrollListener(),this.requestUpdate(),requestAnimationFrame(()=>{var t,i;this._updateMenuPosition(),(i=(t=this.shadowRoot)==null?void 0:t.querySelector(".search-input"))==null||i.focus()})})}_close(){this._open=!1,this._detachScrollListener()}_onFilterInput(e){const t=e.target.value;this._filter=t,this._results=On(t)}_select(e,t){t.stopPropagation(),this._emit(e),this._close(),this._filter=""}_emit(e){this.dispatchEvent(new CustomEvent("icon-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}render(){const e=this.value||this.placeholder,t=Ri(e),i=!!this.value;return h`
      <div class="picker ${this.compact?"compact":""}">
        <ha-picker-field
          .label=${this.label}
          .hasValue=${i}
          .opened=${this._open}
          .compact=${this.compact}
          @click=${this._toggleOpen}
        >
          <ha-svg-icon slot="leading" .path=${t}></ha-svg-icon>
          <span slot="value">${this.value||this.placeholder}</span>
        </ha-picker-field>
        ${this._open?h`
              <div class="menu" style=${P(this._menuStyle)} @click=${r=>r.stopPropagation()}>
                <div class="search">
                  <ha-svg-icon class="search-icon" .path=${Ri("mdi:magnify")}></ha-svg-icon>
                  <input
                    class="search-input"
                    placeholder="搜索 | 添加自定义项目"
                    .value=${this._filter}
                    @input=${this._onFilterInput}
                  />
                </div>
                <ul class="list" role="listbox">
                  ${this._registryReady?this._results.length?this._results.map(r=>h`
                          <li>
                            <button
                              type="button"
                              class="option ${r===this.value?"selected":""}"
                              @click=${o=>this._select(r,o)}
                            >
                              <ha-svg-icon .path=${Ri(r)}></ha-svg-icon>
                              <span>${r}</span>
                            </button>
                          </li>
                        `):h`<li class="empty">未找到匹配的图标</li>`:h`<li class="empty">正在加载图标库…</li>`}
                </ul>
              </div>
            `:m}
      </div>
    `}};qe.styles=w`
    :host {
      display: block;
      min-width: 0;
    }
    .picker {
      position: relative;
      margin-bottom: 12px;
    }
    .picker.compact {
      margin-bottom: 0;
    }
    ha-picker-field {
      width: 100%;
    }
    ha-picker-field ha-svg-icon {
      width: 24px;
      height: 24px;
    }
    .menu {
      background: var(--card-background-color, #fff);
      border-radius: 12px;
      box-shadow: 0 8px 28px rgba(0, 0, 0, 0.16);
      border: 1px solid var(--divider-color);
      overflow: hidden;
      display: flex;
      flex-direction: column;
    }
    .search {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 10px 12px;
      border-bottom: 1px solid var(--divider-color);
      flex-shrink: 0;
    }
    .search-icon {
      width: 20px;
      height: 20px;
      color: var(--secondary-text-color);
      flex-shrink: 0;
    }
    .search-input {
      flex: 1;
      border: none;
      outline: none;
      font: inherit;
      font-size: 14px;
      color: var(--primary-text-color);
      background: transparent;
      min-width: 0;
    }
    .list {
      list-style: none;
      margin: 0;
      padding: 4px 0;
      overflow-y: auto;
      flex: 1;
      min-height: 0;
    }
    .option {
      width: 100%;
      display: flex;
      align-items: center;
      gap: 12px;
      border: none;
      background: transparent;
      padding: 10px 14px;
      cursor: pointer;
      text-align: left;
      font: inherit;
      font-size: 14px;
      color: var(--primary-text-color);
    }
    .option:hover,
    .option.selected {
      background: rgba(3, 169, 244, 0.08);
    }
    .option ha-svg-icon {
      width: 22px;
      height: 22px;
      flex-shrink: 0;
    }
    .option span {
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .empty {
      padding: 16px;
      text-align: center;
      color: var(--secondary-text-color);
      font-size: 13px;
    }
  `;Mt([u()],qe.prototype,"label",2);Mt([u()],qe.prototype,"value",2);Mt([u()],qe.prototype,"placeholder",2);Mt([u({type:Boolean})],qe.prototype,"compact",2);Mt([g()],qe.prototype,"_open",2);Mt([g()],qe.prototype,"_filter",2);Mt([g()],qe.prototype,"_results",2);Mt([g()],qe.prototype,"_registryReady",2);Mt([g()],qe.prototype,"_menuStyle",2);qe=Mt([x("ha-icon-picker")],qe);var P0=Object.defineProperty,A0=Object.getOwnPropertyDescriptor,Le=(e,t,i,r)=>{for(var o=r>1?void 0:r?A0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&P0(t,i,o),o};const Ud={label:"状态颜色"};let ue=class extends y{constructor(){super(...arguments),this.label="颜色",this.value="",this.compact=!1,this.hideHelper=!1,this.includeState=!0,this.includeDefault=!1,this.defaultLabel="默认",this.helper="",this._open=!1,this._custom="#03a9f4",this._showCustom=!1,this._menuStyle={},this._onDocumentClick=e=>{this._open&&(e.composedPath().includes(this)||this._close())},this._repositionMenu=()=>{this._open&&this._updateMenuPosition()}}connectedCallback(){super.connectedCallback(),document.addEventListener("click",this._onDocumentClick),window.addEventListener("resize",this._repositionMenu),this._syncCustom()}disconnectedCallback(){document.removeEventListener("click",this._onDocumentClick),window.removeEventListener("resize",this._repositionMenu),this._detachScrollListener(),super.disconnectedCallback()}updated(e){e.has("value")&&this._syncCustom()}_syncCustom(){this.value&&!Oo.some(e=>e.value===this.value)&&this.value!==""&&(this._custom=this.value)}_attachScrollListener(){this._detachScrollListener();let e=this.parentElement;for(;e;){const t=getComputedStyle(e);if(/(auto|scroll)/.test(t.overflowY)){this._scrollParent=e,e.addEventListener("scroll",this._repositionMenu,{passive:!0});return}e=e.parentElement}}_detachScrollListener(){var e;(e=this._scrollParent)==null||e.removeEventListener("scroll",this._repositionMenu),this._scrollParent=void 0}_updateMenuPosition(){var i;const e=(i=this.shadowRoot)==null?void 0:i.querySelector("ha-picker-field");if(!e)return;const t=pc(e);this._menuStyle={position:"fixed",top:`${t.top}px`,left:`${t.left}px`,width:`${t.width}px`,maxHeight:`${t.maxHeight}px`,zIndex:"500"}}_displayLabel(){return this.value?Rf(this.value):this.includeDefault?this.defaultLabel:Ud.label}_emptySelected(){return!this.value}_showPrimaryOption(){return this.includeState||this.includeDefault}_primaryOptionLabel(){return this.includeDefault?this.defaultLabel:Ud.label}_displayColor(){return this.value?Nt(this.value):"var(--primary-color, #03a9f4)"}_emit(e){this.dispatchEvent(new CustomEvent("color-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}_select(e){this._showCustom=!1,this._close(),this._emit(e)}_applyCustom(){this._close(),this._emit(this._custom)}_toggleOpen(e){if(e.stopPropagation(),this._open){this._close();return}this._open=!0,this._showCustom=!1,this._attachScrollListener(),requestAnimationFrame(()=>this._updateMenuPosition())}_close(){this._open=!1,this._detachScrollListener()}_renderSwatch(e,t=!1){return t?h`<span class="swatch state"></span>`:h`<span class="swatch" style=${`background:${e}`}></span>`}render(){const e=Ri("mdi:palette"),t=!!this.value,i=this._displayColor(),r=t||this.compact||this.includeDefault&&!this.value;return h`
      <div class="picker ${this.compact?"compact":""}">
        <ha-picker-field
          .label=${this.label}
          .hasValue=${r}
          .opened=${this._open}
          .compact=${this.compact}
          @click=${this._toggleOpen}
        >
          ${t&&!Oo.some(o=>o.value===this.value)?h`<span slot="leading" class="swatch leading" style=${`background:${i}`}></span>`:h`<ha-svg-icon slot="leading" .path=${e}></ha-svg-icon>`}
          <span slot="value">${this._displayLabel()}</span>
        </ha-picker-field>

        ${this._open?h`
              <div class="menu" style=${P(this._menuStyle)} @click=${o=>o.stopPropagation()}>
                ${this._showPrimaryOption()?h`
                      <button
                        type="button"
                        class="option ${this._emptySelected()?"selected":""}"
                        @click=${()=>this._select("")}
                      >
                        ${this.includeDefault?h`<span class="swatch default"></span>`:this._renderSwatch("var(--primary-color)",!0)}
                        <span>${this._primaryOptionLabel()}</span>
                      </button>
                    `:m}
                ${Oo.map(o=>h`
                    <button
                      type="button"
                      class="option ${this.value===o.value?"selected":""}"
                      @click=${()=>this._select(o.value)}
                    >
                      ${this._renderSwatch(Nt(o.value))}
                      <span>${o.label}</span>
                    </button>
                  `)}
                <div class="custom-section">
                  <button
                    type="button"
                    class="option custom-toggle"
                    @click=${()=>{this._showCustom=!this._showCustom}}
                  >
                    <ha-svg-icon class="rainbow" .path=${Ri("mdi:palette-swatch")}></ha-svg-icon>
                    <span>自定义颜色</span>
                  </button>
                  ${this._showCustom?h`
                        <div class="custom-row">
                          <input
                            type="color"
                            .value=${this._custom}
                            @input=${o=>{this._custom=o.target.value}}
                          />
                          <input
                            type="text"
                            class="hex"
                            .value=${this._custom}
                            @input=${o=>{this._custom=o.target.value}}
                          />
                          <button type="button" class="apply" @click=${this._applyCustom}>确定</button>
                        </div>
                      `:m}
                </div>
              </div>
            `:m}

        ${this.hideHelper?m:h`<p class="helper">${this.helper||"非活动状态（例如关闭或闭合）将不会被着色。"}</p>`}
      </div>
    `}};ue.styles=w`
    :host {
      display: block;
      min-width: 0;
    }
    .picker {
      margin-bottom: 12px;
    }
    .picker.compact {
      margin-bottom: 0;
    }
    ha-picker-field {
      width: 100%;
    }
    ha-picker-field ha-svg-icon {
      width: 24px;
      height: 24px;
      color: var(--secondary-text-color);
    }
    .swatch {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      flex-shrink: 0;
      border: 1px solid rgba(0, 0, 0, 0.08);
      box-sizing: border-box;
      display: block;
    }
    .swatch.leading {
      width: 20px;
      height: 20px;
    }
    .swatch.state {
      background: linear-gradient(135deg, #bdbdbd 0%, var(--primary-color, #03a9f4) 100%);
    }
    .swatch.default {
      background: var(--ha-section-background-color, var(--secondary-background-color, #e5e5e5));
    }
    .menu {
      background: var(--card-background-color, #fff);
      border-radius: 12px;
      box-shadow: 0 8px 28px rgba(0, 0, 0, 0.16);
      border: 1px solid var(--divider-color);
      overflow: auto;
      padding: 6px;
    }
    .option {
      width: 100%;
      display: flex;
      align-items: center;
      gap: 10px;
      border: none;
      background: transparent;
      padding: 10px 12px;
      border-radius: 8px;
      cursor: pointer;
      font: inherit;
      text-align: left;
      color: var(--primary-text-color);
    }
    .option:hover,
    .option.selected {
      background: rgba(3, 169, 244, 0.08);
    }
    .rainbow {
      width: 22px;
      height: 22px;
      color: var(--secondary-text-color);
    }
    .custom-section {
      border-top: 1px solid var(--divider-color);
      margin-top: 4px;
      padding-top: 4px;
    }
    .custom-row {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 4px 6px 6px;
    }
    .custom-row input[type="color"] {
      width: 36px;
      height: 36px;
      border: none;
      padding: 0;
      background: transparent;
      cursor: pointer;
    }
    .hex {
      flex: 1;
      min-width: 0;
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 6px 8px;
    }
    .apply {
      border: none;
      background: var(--primary-color);
      color: #fff;
      border-radius: 8px;
      padding: 6px 10px;
      font-size: 13px;
      cursor: pointer;
      flex-shrink: 0;
    }
    .helper {
      margin: 6px 0 0;
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
  `;Le([u()],ue.prototype,"label",2);Le([u()],ue.prototype,"value",2);Le([u({type:Boolean})],ue.prototype,"compact",2);Le([u({type:Boolean})],ue.prototype,"hideHelper",2);Le([u({type:Boolean})],ue.prototype,"includeState",2);Le([u({type:Boolean})],ue.prototype,"includeDefault",2);Le([u()],ue.prototype,"defaultLabel",2);Le([u()],ue.prototype,"helper",2);Le([g()],ue.prototype,"_open",2);Le([g()],ue.prototype,"_custom",2);Le([g()],ue.prototype,"_showCustom",2);Le([g()],ue.prototype,"_menuStyle",2);ue=Le([x("ha-color-picker")],ue);var T0=Object.defineProperty,M0=Object.getOwnPropertyDescriptor,co=(e,t,i,r)=>{for(var o=r>1?void 0:r?M0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&T0(t,i,o),o};let ui=class extends y{constructor(){super(...arguments),this.options=[],this.disabled=!1,this.maxColumns=3}_select(e){this.disabled||e===(this.value??"")||this.dispatchEvent(new CustomEvent("value-changed",{detail:{value:e},bubbles:!0,composed:!0}))}render(){const e=Math.min(this.maxColumns,Math.max(this.options.length,1));return h`
      ${this.label?h`<div class="label">${this.label}</div>`:m}
      <div class="list" style="--columns: ${e}">
        ${this.options.map(t=>{const i=t.value===this.value,r=!!(t.disabled||this.disabled);return h`
            <button
              type="button"
              class=${T({option:!0,selected:i})}
              ?disabled=${r}
              @click=${()=>this._select(t.value)}
            >
              <div class="content">
                <span class=${T({radio:!0,checked:i})}></span>
                <div class="text">
                  <span class="option-label">${t.label}</span>
                  ${t.description?h`<span class="description">${t.description}</span>`:m}
                </div>
              </div>
              ${t.image?h`<img class="preview" src=${t.image} alt="" draggable="false" />`:m}
            </button>
          `})}
      </div>
    `}};ui.styles=w`
    :host {
      display: block;
      margin-bottom: 12px;
    }
    .label {
      display: block;
      margin: 0 0 8px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .list {
      display: grid;
      grid-template-columns: repeat(var(--columns, 1), minmax(0, 1fr));
      gap: 12px;
    }
    .option {
      position: relative;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      padding: 12px;
      border: 1px solid var(--divider-color);
      border-radius: 12px;
      background: #fff;
      overflow: hidden;
      cursor: pointer;
      font: inherit;
      color: inherit;
      text-align: left;
      min-height: 0;
    }
    .option::before {
      content: "";
      position: absolute;
      inset: 0;
      background: transparent;
      opacity: 0.2;
      pointer-events: none;
      transition:
        background-color 180ms ease-in-out,
        opacity 180ms ease-in-out;
    }
    .option:hover:not(:disabled)::before {
      background-color: var(--divider-color);
    }
    .option.selected::before {
      background-color: var(--primary-color);
    }
    .option:disabled {
      cursor: not-allowed;
      opacity: 0.55;
    }
    .content {
      position: relative;
      display: flex;
      flex-direction: row;
      align-items: center;
      gap: 8px;
      width: 100%;
      min-width: 0;
      z-index: 1;
    }
    .radio {
      flex: none;
      width: 18px;
      height: 18px;
      border: 2px solid var(--secondary-text-color);
      border-radius: 50%;
      box-sizing: border-box;
      position: relative;
    }
    .radio.checked {
      border-color: var(--primary-color);
    }
    .radio.checked::after {
      content: "";
      position: absolute;
      inset: 3px;
      border-radius: 50%;
      background: var(--primary-color);
    }
    .text {
      display: flex;
      flex-direction: column;
      gap: 2px;
      min-width: 0;
      flex: 1;
    }
    .option-label {
      color: var(--primary-text-color);
      font-size: 14px;
      line-height: 1.25;
      overflow: hidden;
      white-space: nowrap;
      text-overflow: ellipsis;
    }
    .description {
      color: var(--secondary-text-color);
      font-size: 12px;
      line-height: 1.25;
    }
    .preview {
      position: relative;
      z-index: 1;
      max-width: 96px;
      max-height: 96px;
      margin: auto;
      pointer-events: none;
      user-select: none;
    }
  `;co([u({attribute:!1})],ui.prototype,"options",2);co([u()],ui.prototype,"value",2);co([u()],ui.prototype,"label",2);co([u({type:Boolean})],ui.prototype,"disabled",2);co([u({type:Number,attribute:"max_columns"})],ui.prototype,"maxColumns",2);ui=co([x("ha-select-box")],ui);function jd(e,t){const i=e===void 0?t:e;return typeof i=="boolean"?i?"true":"false":i===null?"null":String(i)}const O0=new Set(["heading","picture-elements","path-badge","error","color"]),L0=["slider","climate","cover","switch","tile","gain","multi-fader","xy-pad","trigger","media","mic","rgba","hsv","sensor","label","status","clock","bar","line2d","scatter2d","line3d","scatter3d","markdown","link"];function D0(){const e=gh().filter(i=>!O0.has(i.type)),t=new Map(L0.map((i,r)=>[i,r]));return e.sort((i,r)=>{const o=t.get(i.type)??1e3,n=t.get(r.type)??1e3;return o!==n?o-n:i.name.localeCompare(r.name,"zh")}),e.map(i=>({type:i.type,label:i.name}))}function Ia(e,t,i){var s,a;const r=fh(e),o=((a=(s=r==null?void 0:r.constructor)==null?void 0:s.getStubConfig)==null?void 0:a.call(s))??{type:e},n=t||String((i==null?void 0:i.entity)??o.entity??"").trim();return i&&i.type===e?{...i,type:e,...n?{entity:n}:{}}:{...o,type:e,...n?{entity:n}:{}}}function Jt(e,t,i,r){var s;const o={action:e,...t},n={tap_action:o,action:e,press_value:o.press_value,release_value:o.release_value,card:o.card};if(e==="open-card"){const a=String(((s=o.card)==null?void 0:s.entity)??(r==null?void 0:r.entity)??"").trim();a&&(n.entity=a)}(r==null?void 0:r.entity)!==void 0&&e!=="open-card"&&(n.entity=r.entity),i(n)}function I0(e,t){const i=e??{},r=jo(i),o=String(i.entity??"").trim(),n=r.action,s=r.card??Ia("slider",o),a=String(s.type??"slider").trim()||"slider",l=D0();return h`
    <label class="field">
      <span>按下行为</span>
      <select
        @change=${c=>{const d=c.target.value;if(d==="switch"){Jt(d,{},t);return}if(d==="rw"){Jt(d,{press_value:r.press_value??!0,release_value:r.release_value??!1},t);return}if(d==="open-card"){const p=Ia(a,o,s);Jt(d,{card:p},t,{entity:String(p.entity??o).trim()});return}Jt("trigger",{},t)}}
      >
        <option value="trigger" ?selected=${n==="trigger"}>触发（自动弹起）</option>
        <option value="switch" ?selected=${n==="switch"}>开关（保持按下）</option>
        <option value="rw" ?selected=${n==="rw"}>读写（保持按下）</option>
        <option value="open-card" ?selected=${n==="open-card"}>弹出卡片</option>
      </select>
      <p class="helper">
        ${n==="trigger"?"单击发送 true，随后自动弹起。读写地址请填「绑定地址」。":n==="switch"?"单击切换 true/false；跟随实体状态。读写地址请填「绑定地址」。":n==="rw"?"单击切换按下/弹起值；接收值匹配时同步外观。读写地址请填「绑定地址」。":n==="open-card"?"单击只弹出卡片，不向徽章写值。读写地址请在下方卡片设置中配置。":""}
      </p>
    </label>
    ${n==="rw"?h`
          <label class="field">
            <span>按下发送值</span>
            <input
              .value=${jd(r.press_value,!0)}
              placeholder="true"
              @input=${c=>Jt("rw",{press_value:Dr(c.target.value),release_value:r.release_value??!1},t)}
            />
          </label>
          <label class="field">
            <span>弹起发送值</span>
            <input
              .value=${jd(r.release_value,!1)}
              placeholder="false"
              @input=${c=>Jt("rw",{press_value:r.press_value??!0,release_value:Dr(c.target.value)},t)}
            />
            <p class="helper">
              再点一次发送弹起值。若实体当前值等于按下或弹起值，徽章状态会同步更新。
            </p>
          </label>
        `:m}
    ${n==="open-card"?h`
          <label class="field">
            <span>弹出卡片类型</span>
            <select
              @change=${c=>{const d=c.target.value,p=Ia(d,o,s);Jt("open-card",{card:p},t,{entity:String(p.entity??"").trim()})}}
            >
              ${l.map(c=>h`
                  <option value=${c.type} ?selected=${a===c.type}>
                    ${c.label}
                  </option>
                `)}
              ${l.some(c=>c.type===a)?m:h`<option value=${a} selected>${a}</option>`}
            </select>
          </label>
          <p class="helper">卡片的读写地址、范围与样式只在下方配置，与徽章绑定地址无关。</p>
          <div
            class="open-card-editor"
            @config-changed=${c=>{c.stopPropagation();const d=c.detail;d!=null&&d.config&&Jt("open-card",{card:d.config},t,{entity:String(d.config.entity??"").trim()})}}
          >
            <flow-card-config-editor .config=${s}></flow-card-config-editor>
          </div>
        `:m}
  `}function qd(e,t,i){const r=(i==null?void 0:i.textLabel)??"显示文字",o=fn(e,i==null?void 0:i.sizeFallback),n=Ff(o);return h`
    <label class="switch-row">
      <span>${r}</span>
      <input
        type="checkbox"
        .checked=${Rt(e)}
        @change=${s=>{const a=s.target.checked;t({show_name:a,show_state:a})}}
      />
    </label>
    <label class="switch-row">
      <span>显示底色</span>
      <input
        type="checkbox"
        .checked=${di(e)}
        @change=${s=>t({show_background:s.target.checked})}
      />
    </label>
    <p class="helper">关闭底色后无胶囊填充；图标颜色随实体/按下状态变化。</p>
    <label class="field">
      <span>大小</span>
      <select
        @change=${s=>t({size:Number(s.target.value)})}
      >
        ${Fa.map(s=>h`
            <option value=${s.value} ?selected=${n===s.value}>
              ${s.label}（高 ${s.value}px）
            </option>
          `)}
      </select>
    </label>
  `}var H0=Object.defineProperty,z0=Object.getOwnPropertyDescriptor,$i=(e,t,i,r)=>{for(var o=r>1?void 0:r?z0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&H0(t,i,o),o};let yt=class extends y{constructor(){super(...arguments),this.mode="heading",this.label="徽章",this.hint="",this.emptyText="可添加状态徽章（只读）或控制徽章（点击操作）",this.badges=[],this._selectedIndex=0,this._openIndex=0}get selectedIndex(){return this._selectedIndex>=0&&this.badges[this._selectedIndex]?this._selectedIndex:this.badges.length>0?0:-1}selectIndex(e){e<0||e>=this.badges.length||this._select(e,!0)}updated(e){if(e.has("badges")){if(this.badges.length===0){this._selectedIndex=-1,this._openIndex=null;return}(this._selectedIndex<0||this._selectedIndex>=this.badges.length)&&(this._selectedIndex=Math.min(Math.max(0,this._selectedIndex),this.badges.length-1),this._openIndex!==null&&(this._openIndex=this._selectedIndex))}}applyPicturePosition(e,t){if(this.mode!=="picture")return;const i=this.selectedIndex;i<0||this._emit(this.badges.map((r,o)=>o===i?ts(r,t,e):r))}_emit(e){this.dispatchEvent(new CustomEvent("badges-changed",{bubbles:!0,composed:!0,detail:{badges:e}}))}_emitSelection(){const e=this.selectedIndex;this.dispatchEvent(new CustomEvent("badge-selected",{bubbles:!0,composed:!0,detail:{index:e,mode:this.mode}})),this.mode==="picture"&&document.dispatchEvent(new CustomEvent("flow-picture-badge-highlight",{detail:{index:e}}))}_select(e,t=!0){e<0||e>=this.badges.length||(this._selectedIndex=e,t&&(this._openIndex=e),this._emitSelection(),this.updateComplete.then(()=>this._scrollSelectedIntoView()))}_scrollSelectedIntoView(){const e=this.renderRoot.querySelector(`.chip[data-index="${this.selectedIndex}"]`);e==null||e.scrollIntoView({inline:"nearest",block:"nearest",behavior:"smooth"})}_entityIcon(e){return e==="button"?"mdi:gesture-tap-button":this.mode==="path"?"mdi:map-marker":"mdi:thermometer"}_createStub(e){const t={};return this.mode==="picture"&&(t.style=e==="button"?{top:"50%",left:"50%"}:{top:"40%",left:"40%"}),this.mode==="path"&&e==="entity"?Kn({icon:this._entityIcon("entity"),color:"accent",show_icon:!0,show_name:!1,show_state:!1,...t}):e==="button"?Va({icon:this._entityIcon("button"),...t}):Kn({icon:this._entityIcon("entity"),...t})}_add(e){const t=[...this.badges,this._createStub(e)],i=t.length-1;this._selectedIndex=i,this._openIndex=i,this._emit(t),this.updateComplete.then(()=>{this._emitSelection(),this._scrollSelectedIntoView()})}_remove(e){const t=this.badges.filter((r,o)=>o!==e),i=t.length===0?-1:Math.min(e,t.length-1);this._selectedIndex=i,this._openIndex=i<0?null:i,this._emit(t),this.updateComplete.then(()=>this._emitSelection())}_move(e,t){const i=e+t;if(i<0||i>=this.badges.length)return;const r=[...this.badges],[o]=r.splice(e,1);r.splice(i,0,o),this._selectedIndex=i,this._openIndex=i,this._emit(r),this.updateComplete.then(()=>this._emitSelection())}_duplicate(e){var o,n;const t=this.badges[e];if(!t)return;let i={...t};if(this.mode==="picture"){const s=t;i=ts({...s},Math.min(90,dt((o=s.style)==null?void 0:o.top,50)+6),Math.min(90,dt((n=s.style)==null?void 0:n.left,50)+6))}const r=[...this.badges.slice(0,e+1),i,...this.badges.slice(e+1)];this._selectedIndex=e+1,this._openIndex=e+1,this._emit(r),this.updateComplete.then(()=>{this._emitSelection(),this._scrollSelectedIntoView()})}_patch(e,t){this._emit(this.badges.map((i,r)=>r===e?uf(i,t):i))}_changeType(e,t){const i=this.badges[e];if(!i)return;const r={entity:i.entity,icon:i.icon||this._entityIcon(t),color:i.color,size:i.size,show_background:i.show_background,style:i.style,attribute:i.attribute,name:i.name,unit:i.unit};if(t==="button"){this._patch(e,Va({...r,text:String(i.text??i.name??"触发"),show_name:i.show_name??!0}));return}this._patch(e,Kn({...r,show_icon:i.show_icon??!0,show_name:i.show_name??i.show_state??this.mode!=="path"}))}_setPos(e,t,i){var s,a;const r=this.badges[e];if(!r)return;const o=dt(t==="top"?i:(s=r.style)==null?void 0:s.top,50),n=dt(t==="left"?i:(a=r.style)==null?void 0:a.left,50);this._emit(this.badges.map((l,c)=>c===e?ts(l,o,n):l))}_title(e,t){const i=oi(e)?"控制徽章":"状态徽章",r=String(e.name||e.text||"").trim();return r||`${i} ${t+1}`}_subtitle(e){const t=jo(e);if(oi(e)&&t.action==="open-card"){const i=t.card,r=String((i==null?void 0:i.type)??"卡片").trim()||"卡片",o=String((i==null?void 0:i.entity)??e.entity??"").trim();return o?`弹出 ${r} · ${o}`:`弹出 ${r}`}return String(e.entity||e.text||e.name||"未选择地址")}_renderBadgeEntityField(e,t,i){const r=i==="button"?jo(e):void 0,o=(r==null?void 0:r.action)==="open-card";return o&&this.mode!=="path"?m:h`
      <ha-entity-picker
        label=${o?"轨迹位置地址":"绑定地址"}
        .value=${String(e.entity??"")}
        ?optional=${i==="button"}
        @value-changed=${s=>{var l,c;const a=s.detail.value;if(o){const d={...(r==null?void 0:r.card)??{type:"slider"},type:String(((l=r==null?void 0:r.card)==null?void 0:l.type)??"slider"),...String(((c=r==null?void 0:r.card)==null?void 0:c.entity)??"").trim()?{}:a?{entity:a}:{}};this._patch(t,{entity:a,tap_action:{action:"open-card",card:d},action:"open-card",card:d});return}this._patch(t,{entity:a})}}
      ></ha-entity-picker>
      ${o?h`<p class="helper">仅用于徽章在轨迹上的位置，不负责弹出卡片里的读写。</p>`:m}
    `}_renderCommonFields(e,t,i){return h`
      <label class="field">
        <span>类型</span>
        <select
          @change=${r=>this._changeType(t,r.target.value)}
        >
          <option value="entity" ?selected=${i==="entity"}>状态徽章</option>
          <option value="button" ?selected=${i==="button"}>控制徽章</option>
        </select>
      </label>
      ${i==="entity"?this._renderBadgeEntityField(e,t,i):m}
      ${this.mode==="path"?h`
            <label class="field">
              <span>名称</span>
              <input
                .value=${String(e.name??"")}
                placeholder="留空则使用实体名"
                @input=${r=>this._patch(t,{name:r.target.value})}
              />
            </label>
            <label class="field">
              <span>位置属性名</span>
              <input
                .value=${String(e.attribute??"")}
                placeholder="progress"
                @input=${r=>this._patch(t,{attribute:r.target.value})}
              />
              <p class="helper">可选：用该属性值计算轨迹位置；留空则使用实体状态。</p>
            </label>
          `:m}
      <div class="appearance">
        <ha-icon-picker
          label="图标"
          .value=${String(e.icon??"")}
          placeholder=${this._entityIcon(i)}
          @icon-changed=${r=>this._patch(t,{icon:r.detail.value})}
        ></ha-icon-picker>
        <ha-color-picker
          label="颜色"
          .value=${String(e.color??"")}
          ?includeState=${i==="entity"}
          @color-changed=${r=>this._patch(t,{color:r.detail.value})}
        ></ha-color-picker>
      </div>
    `}_renderEntityFields(e,t){const i=this.mode!=="heading";return h`
      <label class="switch-row">
        <span>显示图标</span>
        <input
          type="checkbox"
          .checked=${e.show_icon!==!1}
          @change=${r=>this._patch(t,{show_icon:r.target.checked})}
        />
      </label>
      ${i?qd(e,r=>this._patch(t,r),{textLabel:this.mode==="path"?"显示状态值":"显示文字"}):h`
            <label class="switch-row">
              <span>显示文字</span>
              <input
                type="checkbox"
                .checked=${Rt(e)}
                @change=${r=>{const o=r.target.checked;this._patch(t,{show_name:o,show_state:o})}}
              />
            </label>
            <label class="switch-row">
              <span>显示底色</span>
              <input
                type="checkbox"
                .checked=${di(e)}
                @change=${r=>this._patch(t,{show_background:r.target.checked})}
              />
            </label>
          `}
      ${this.mode!=="heading"?h`
            <label class="field">
              <span>单位</span>
              <input
                .value=${String(e.unit??"")}
                placeholder=${this.mode==="path"?"°C":"kWh"}
                @input=${r=>this._patch(t,{unit:r.target.value})}
              />
            </label>
          `:m}
    `}_renderButtonFields(e,t){const i=this.mode!=="heading";return h`
      ${this.mode==="path"?this._renderBadgeEntityField(e,t,"button"):m}
      ${I0(e,r=>this._patch(t,r))}
      ${this.mode!=="path"?this._renderBadgeEntityField(e,t,"button"):m}
      <label class="field">
        <span>文字</span>
        <input
          .value=${String(e.text??e.name??"")}
          placeholder="触发"
          @input=${r=>this._patch(t,{text:r.target.value})}
        />
      </label>
      ${i?qd(e,r=>this._patch(t,r),{textLabel:"显示文字"}):h`
            <label class="switch-row">
              <span>显示文字</span>
              <input
                type="checkbox"
                .checked=${Rt(e)}
                @change=${r=>this._patch(t,{show_name:r.target.checked})}
              />
            </label>
            <label class="switch-row">
              <span>显示底色</span>
              <input
                type="checkbox"
                .checked=${di(e)}
                @change=${r=>this._patch(t,{show_background:r.target.checked})}
              />
            </label>
          `}
    `}_renderPicturePosition(e,t){var r,o;if(this.mode!=="picture")return m;const i=e;return h`
      <div class="pos">
        <label class="field">
          <span>上 (%)</span>
          <input
            type="number"
            min="0"
            max="100"
            step="0.5"
            .value=${String(dt((r=i.style)==null?void 0:r.top,50))}
            @input=${n=>this._setPos(t,"top",n.target.value)}
          />
        </label>
        <label class="field">
          <span>左 (%)</span>
          <input
            type="number"
            min="0"
            max="100"
            step="0.5"
            .value=${String(dt((o=i.style)==null?void 0:o.left,50))}
            @input=${n=>this._setPos(t,"left",n.target.value)}
          />
        </label>
      </div>
    `}render(){const e=this.selectedIndex,t=e>=0?this.badges[e]:void 0,i=t&&oi(t)?"button":"entity";return h`
      <div class="badges-editor">
        <div class="head">
          <span class="label">${this.label}</span>
          <div class="add-group">
            <button type="button" class="add" @click=${()=>this._add("entity")}>
              + 状态徽章
            </button>
            <button type="button" class="add" @click=${()=>this._add("button")}>
              + 控制徽章
            </button>
          </div>
        </div>
        ${this.hint?h`<p class="hint">${this.hint}</p>`:m}
        ${this.badges.length===0?h`<p class="empty">${this.emptyText}</p>`:h`
              <div class="chip-row" role="tablist" aria-label=${this.label}>
                ${this.badges.map((r,o)=>{const n=oi(r)?"button":"entity",s=e===o;return h`
                    <button
                      type="button"
                      role="tab"
                      class="chip ${s?"selected":""} ${n}"
                      data-index=${o}
                      aria-selected=${s?"true":"false"}
                      title=${this._subtitle(r)}
                      @click=${()=>this._select(o,!0)}
                    >
                      <span class="chip-index">${o+1}</span>
                      <span class="chip-type">${n==="button"?"控制":"状态"}</span>
                      <span class="chip-name">${this._title(r,o)}</span>
                    </button>
                  `})}
              </div>
              ${t?h`
                    <div class="detail" data-index=${e}>
                      <div class="detail-toolbar">
                        <span class="detail-title">
                          #${e+1}
                          · ${i==="button"?"控制徽章":"状态徽章"}
                        </span>
                        <div class="actions">
                          <button
                            type="button"
                            title="上移"
                            ?disabled=${e<=0}
                            @click=${()=>this._move(e,-1)}
                          >
                            ↑
                          </button>
                          <button
                            type="button"
                            title="下移"
                            ?disabled=${e>=this.badges.length-1}
                            @click=${()=>this._move(e,1)}
                          >
                            ↓
                          </button>
                          <button
                            type="button"
                            title="复制"
                            @click=${()=>this._duplicate(e)}
                          >
                            <ha-icon icon="mdi:content-copy"></ha-icon>
                          </button>
                          <button
                            type="button"
                            class="danger"
                            title="删除"
                            @click=${()=>this._remove(e)}
                          >
                            <ha-icon icon="mdi:delete"></ha-icon>
                          </button>
                        </div>
                      </div>
                      ${this._renderCommonFields(t,e,i)}
                      ${i==="entity"?this._renderEntityFields(t,e):this._renderButtonFields(t,e)}
                      ${this._renderPicturePosition(t,e)}
                    </div>
                  `:m}
            `}
      </div>
    `}};yt.styles=w`
    .badges-editor {
      margin-bottom: 12px;
    }
    .head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 4px;
    }
    .label {
      font-size: 13px;
      font-weight: 600;
    }
    .add-group {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
      justify-content: flex-end;
    }
    .add {
      border: none;
      background: transparent;
      color: var(--primary-color);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 4px 0;
    }
    .hint,
    .empty,
    .helper {
      margin: 0 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
    }
    .chip-row {
      display: flex;
      flex-direction: row;
      flex-wrap: nowrap;
      gap: 8px;
      overflow-x: auto;
      padding: 2px 2px 10px;
      margin-bottom: 4px;
      scrollbar-width: thin;
    }
    .chip {
      flex: 0 0 auto;
      display: inline-flex;
      align-items: center;
      gap: 6px;
      max-width: 160px;
      height: 32px;
      padding: 0 10px 0 6px;
      border: 1px solid var(--divider-color);
      border-radius: 999px;
      background: var(--card-background-color, #fff);
      color: var(--primary-text-color);
      cursor: pointer;
      font: inherit;
      font-size: 12px;
      line-height: 1;
    }
    .chip:hover {
      border-color: color-mix(in srgb, var(--primary-color) 45%, var(--divider-color));
    }
    .chip.selected {
      border-color: var(--primary-color);
      background: color-mix(in srgb, var(--primary-color) 12%, transparent);
      box-shadow: 0 0 0 1px color-mix(in srgb, var(--primary-color) 30%, transparent);
    }
    .chip-index {
      width: 18px;
      height: 18px;
      border-radius: 50%;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      font-size: 10px;
      font-weight: 700;
      color: var(--secondary-text-color);
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
    }
    .chip.selected .chip-index {
      color: #fff;
      background: var(--primary-color);
    }
    .chip-type {
      flex-shrink: 0;
      font-size: 10px;
      font-weight: 600;
      padding: 1px 5px;
      border-radius: 999px;
    }
    .chip.entity .chip-type {
      color: #0369a1;
      background: color-mix(in srgb, #0ea5e9 16%, transparent);
    }
    .chip.button .chip-type {
      color: #a16207;
      background: color-mix(in srgb, #eab308 18%, transparent);
    }
    .chip-name {
      min-width: 0;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      font-weight: 600;
    }
    .detail {
      border: 1px solid var(--divider-color);
      border-radius: 10px;
      padding: 10px 12px 12px;
      background: var(--secondary-background-color, #f8f9fa);
    }
    .detail-toolbar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 10px;
    }
    .detail-title {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 2px;
    }
    .actions button {
      border: none;
      background: transparent;
      color: var(--secondary-text-color);
      cursor: pointer;
      padding: 4px;
      border-radius: 6px;
      width: 28px;
      height: 28px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
    }
    .actions button:hover:not(:disabled) {
      background: color-mix(in srgb, var(--primary-color) 10%, transparent);
      color: var(--primary-text-color);
    }
    .actions button:disabled {
      opacity: 0.35;
      cursor: default;
    }
    .actions button.danger {
      color: var(--error-color, #b91c1c);
    }
    .actions button.danger:hover {
      background: color-mix(in srgb, var(--error-color, #d32f2f) 10%, transparent);
      color: var(--error-color, #d32f2f);
    }
    ha-icon {
      --mdc-icon-size: 16px;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 10px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field span {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field input,
    .field select {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .open-card-editor {
      margin-top: 4px;
      padding: 10px;
      border: 1px dashed var(--divider-color);
      border-radius: 10px;
      background: var(--card-background-color, #fff);
    }
    .appearance {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 16px;
      align-items: start;
      margin-bottom: 4px;
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 10px;
      font-size: 13px;
      color: var(--primary-text-color);
      cursor: pointer;
    }
    .switch-row span {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .switch-row input[type="checkbox"] {
      width: 18px;
      height: 18px;
      accent-color: var(--primary-color);
    }
    .pos {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 12px;
    }
    @media (max-width: 520px) {
      .appearance,
      .pos {
        grid-template-columns: 1fr;
      }
    }
  `;$i([u()],yt.prototype,"mode",2);$i([u()],yt.prototype,"label",2);$i([u()],yt.prototype,"hint",2);$i([u()],yt.prototype,"emptyText",2);$i([u({attribute:!1})],yt.prototype,"badges",2);$i([g()],yt.prototype,"_selectedIndex",2);$i([g()],yt.prototype,"_openIndex",2);yt=$i([x("flow-badges-list-editor")],yt);var V0=Object.defineProperty,N0=Object.getOwnPropertyDescriptor,eu=(e,t,i,r)=>{for(var o=r>1?void 0:r?N0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&V0(t,i,o),o};let Ps=class extends y{constructor(){super(...arguments),this.badges=[]}_onChanged(e){e.stopPropagation(),this.dispatchEvent(new CustomEvent("badges-changed",{bubbles:!0,composed:!0,detail:{badges:e.detail.badges}}))}render(){return h`
      <flow-badges-list-editor
        mode="heading"
        label="徽章"
        emptyText="可添加状态徽章（只读）或控制徽章（点击操作）"
        .badges=${this.badges}
        @badges-changed=${this._onChanged}
      ></flow-badges-list-editor>
    `}};Ps.styles=w`
    :host {
      display: block;
    }
  `;eu([u({attribute:!1})],Ps.prototype,"badges",2);Ps=eu([x("flow-heading-badges-editor")],Ps);var R0=Object.defineProperty,F0=Object.getOwnPropertyDescriptor,tu=(e,t,i,r)=>{for(var o=r>1?void 0:r?F0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&R0(t,i,o),o};let As=class extends y{constructor(){super(...arguments),this.entities=[]}_emit(e){this.dispatchEvent(new CustomEvent("entities-changed",{bubbles:!0,composed:!0,detail:{entities:e}}))}_add(){this._emit([...this.entities,{entity:"",name:""}])}_remove(e){this._emit(this.entities.filter((t,i)=>i!==e))}_patch(e,t){this._emit(this.entities.map((i,r)=>r===e?{...i,...t}:i))}render(){return h`
      <div class="editor">
        <div class="head">
          <span class="label">柱（每柱绑定一个动作）</span>
          <button type="button" class="add" @click=${this._add}>+ 添加柱</button>
        </div>
        ${this.entities.length===0?h`<p class="empty">添加动作后，每根柱显示对应动作的实时数值</p>`:this.entities.map((e,t)=>h`
                <div class="row">
                  <span class="index">${t+1}</span>
                  <ha-entity-picker
                    compact
                    label=""
                    placeholder="动作"
                    .value=${e.entity??""}
                    @value-changed=${i=>{i.stopPropagation(),this._patch(t,{entity:i.detail.value,...i.detail.actionName?{name:i.detail.actionName}:{}})}}
                  ></ha-entity-picker>
                  <input
                    class="name"
                    type="text"
                    .value=${e.name??""}
                    placeholder="名称"
                    @input=${i=>this._patch(t,{name:i.target.value})}
                  />
                  <ha-color-picker
                    compact
                    hideHelper
                    label="颜色"
                    .value=${e.color??""}
                    @color-changed=${i=>{i.stopPropagation(),this._patch(t,{color:i.detail.value})}}
                  ></ha-color-picker>
                  <button
                    type="button"
                    class="remove"
                    aria-label="删除"
                    title="删除"
                    @click=${()=>this._remove(t)}
                  >
                    ×
                  </button>
                </div>
              `)}
      </div>
    `}};As.styles=w`
    .editor {
      margin-bottom: 12px;
    }
    .head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 8px;
    }
    .label {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .add {
      border: none;
      background: transparent;
      color: var(--primary-color, #03a9f4);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 0;
    }
    .empty {
      margin: 0 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
    }
    .row {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 6px 0;
      border-top: 1px solid var(--divider-color);
    }
    .index {
      flex: 0 0 20px;
      font-size: 12px;
      color: var(--secondary-text-color);
      text-align: center;
    }
    .row ha-entity-picker {
      flex: 1 1 45%;
      min-width: 96px;
    }
    .name {
      flex: 0 1 88px;
      min-width: 64px;
      box-sizing: border-box;
      height: 36px;
      font: inherit;
      font-size: 13px;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 0 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .row ha-color-picker {
      flex: 0 1 108px;
      min-width: 80px;
    }
    .remove {
      flex: 0 0 28px;
      width: 28px;
      height: 28px;
      border: none;
      border-radius: 6px;
      background: transparent;
      color: var(--secondary-text-color);
      font-size: 20px;
      line-height: 1;
      cursor: pointer;
      padding: 0;
    }
    .remove:hover {
      color: var(--error-color, #db4437);
      background: rgba(219, 68, 55, 0.08);
    }
    @media (max-width: 520px) {
      .row {
        flex-wrap: wrap;
      }
      .row ha-entity-picker {
        flex: 1 1 100%;
      }
    }
  `;tu([u({attribute:!1})],As.prototype,"entities",2);As=tu([x("flow-bar-entities-editor")],As);var B0=Object.defineProperty,U0=Object.getOwnPropertyDescriptor,iu=(e,t,i,r)=>{for(var o=r>1?void 0:r?U0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&B0(t,i,o),o};let Ts=class extends y{constructor(){super(...arguments),this.entities=[]}_emit(e){this.dispatchEvent(new CustomEvent("entities-changed",{bubbles:!0,composed:!0,detail:{entities:e}}))}_add(){this._emit([...this.entities,{entity:"",name:"",icon:""}])}_remove(e){this._emit(this.entities.filter((t,i)=>i!==e))}_patch(e,t){this._emit(this.entities.map((i,r)=>r===e?{...i,...t}:i))}render(){return h`
      <div class="editor">
        <div class="head">
          <span class="label">状态项（每项绑定一个动作）</span>
          <button type="button" class="add" @click=${this._add}>+ 添加状态</button>
        </div>
        ${this.entities.length===0?h`<p class="empty">添加动作后，每行显示对应动作的实时状态</p>`:this.entities.map((e,t)=>h`
                <div class="row">
                  <span class="index">${t+1}</span>
                  <ha-entity-picker
                    compact
                    label=""
                    placeholder="动作"
                    .value=${e.entity??""}
                    @value-changed=${i=>{i.stopPropagation(),this._patch(t,{entity:i.detail.value,...i.detail.actionName?{name:i.detail.actionName}:{}})}}
                  ></ha-entity-picker>
                  <input
                    class="name"
                    type="text"
                    .value=${e.name??""}
                    placeholder="名称"
                    @input=${i=>this._patch(t,{name:i.target.value})}
                  />
                  <ha-icon-picker
                    compact
                    label="图标"
                    .value=${e.icon??""}
                    @icon-changed=${i=>{i.stopPropagation(),this._patch(t,{icon:i.detail.value})}}
                  ></ha-icon-picker>
                  <button
                    type="button"
                    class="remove"
                    aria-label="删除"
                    title="删除"
                    @click=${()=>this._remove(t)}
                  >
                    ×
                  </button>
                </div>
              `)}
      </div>
    `}};Ts.styles=w`
    .editor {
      margin-bottom: 12px;
    }
    .head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 8px;
    }
    .label {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .add {
      border: none;
      background: transparent;
      color: var(--primary-color, #03a9f4);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 0;
    }
    .empty {
      margin: 0 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
    }
    .row {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 6px 0;
      border-top: 1px solid var(--divider-color);
    }
    .index {
      flex: 0 0 20px;
      font-size: 12px;
      color: var(--secondary-text-color);
      text-align: center;
    }
    .row ha-entity-picker {
      flex: 1 1 45%;
      min-width: 96px;
    }
    .name {
      flex: 0 1 88px;
      min-width: 64px;
      box-sizing: border-box;
      height: 36px;
      font: inherit;
      font-size: 13px;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 0 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .row ha-icon-picker {
      flex: 0 1 108px;
      min-width: 80px;
    }
    .remove {
      flex: 0 0 28px;
      width: 28px;
      height: 28px;
      border: none;
      border-radius: 6px;
      background: transparent;
      color: var(--secondary-text-color);
      font-size: 20px;
      line-height: 1;
      cursor: pointer;
      padding: 0;
    }
    .remove:hover {
      color: var(--error-color, #db4437);
      background: rgba(219, 68, 55, 0.08);
    }
    @media (max-width: 520px) {
      .row {
        flex-wrap: wrap;
      }
      .row ha-entity-picker {
        flex: 1 1 100%;
      }
    }
  `;iu([u({attribute:!1})],Ts.prototype,"entities",2);Ts=iu([x("flow-status-entities-editor")],Ts);var j0=Object.defineProperty,q0=Object.getOwnPropertyDescriptor,fc=(e,t,i,r)=>{for(var o=r>1?void 0:r?q0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&j0(t,i,o),o};let rn=class extends y{constructor(){super(...arguments),this.elements=[],this._mode="picture"}get selectedIndex(){var e;return((e=this._list)==null?void 0:e.selectedIndex)??-1}applyPosition(e,t){var i;(i=this._list)==null||i.applyPicturePosition(e,t)}selectIndex(e){var t;(t=this._list)==null||t.selectIndex(e)}_onChanged(e){e.stopPropagation(),this.dispatchEvent(new CustomEvent("elements-changed",{bubbles:!0,composed:!0,detail:{elements:e.detail.badges}}))}render(){return h`
      <flow-badges-list-editor
        .mode=${this._mode}
        label="元素"
        hint="点上方徽章切换编辑；可在右侧预览图上点击或拖动定位。"
        emptyText="还没有元素。添加后会叠在底图上。"
        .badges=${this.elements}
        @badges-changed=${this._onChanged}
      ></flow-badges-list-editor>
    `}};rn.styles=w`
    :host {
      display: block;
    }
  `;fc([u({attribute:!1})],rn.prototype,"elements",2);fc([mi("flow-badges-list-editor")],rn.prototype,"_list",2);rn=fc([x("flow-picture-elements-editor")],rn);var G0=Object.defineProperty,X0=Object.getOwnPropertyDescriptor,gc=(e,t,i,r)=>{for(var o=r>1?void 0:r?X0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&G0(t,i,o),o};let on=class extends y{constructor(){super(...arguments),this.badges=[],this._mode="path"}get selectedIndex(){var e;return((e=this._list)==null?void 0:e.selectedIndex)??-1}_onChanged(e){e.stopPropagation(),this.dispatchEvent(new CustomEvent("badges-changed",{bubbles:!0,composed:!0,detail:{badges:e.detail.badges}}))}render(){return h`
      <flow-badges-list-editor
        .mode=${this._mode}
        label="轨迹徽章"
        hint="状态徽章与控制徽章共用同一条轨迹；每个徽章可绑定各自的实体、显示方式和进度来源。"
        emptyText="还没有徽章。至少添加一个徽章后，轨迹上才会显示运动标记。"
        .badges=${this.badges}
        @badges-changed=${this._onChanged}
      ></flow-badges-list-editor>
    `}};on.styles=w`
    :host {
      display: block;
    }
  `;gc([u({attribute:!1})],on.prototype,"badges",2);gc([mi("flow-badges-list-editor")],on.prototype,"_list",2);on=gc([x("flow-path-badges-editor")],on);var W0=Object.defineProperty,Y0=Object.getOwnPropertyDescriptor,ki=(e,t,i,r)=>{for(var o=r>1?void 0:r?Y0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&W0(t,i,o),o};const Gd="http://www.w3.org/2000/svg",Xd=[{x:10,y:88},{x:28,y:18},{x:72,y:18},{x:90,y:88}];function Wd(e){return Math.min(100,Math.max(0,e))}function Fi(e){return{x:Number(Wd(Number(e.x)).toFixed(2)),y:Number(Wd(Number(e.y)).toFixed(2))}}function Yd(e){if(e.length===0)return"";if(e.length===1){const n=Fi(e[0]);return`M ${n.x} ${n.y}`}if(e.length===2){const[n,s]=e.map(Fi);return`M ${n.x} ${n.y} L ${s.x} ${s.y}`}const t=e.map(Fi);let i=`M ${t[0].x} ${t[0].y}`;for(let n=1;n<t.length-1;n+=1){const s=t[n],a=t[n+1],l=Number(((s.x+a.x)/2).toFixed(2)),c=Number(((s.y+a.y)/2).toFixed(2));i+=` Q ${s.x} ${s.y} ${l} ${c}`}const r=t[t.length-2],o=t[t.length-1];return i+=` Q ${r.x} ${r.y} ${o.x} ${o.y}`,i}function Z0(e,t=12){if(!e.trim())return[];try{const i=document.createElementNS(Gd,"svg"),r=document.createElementNS(Gd,"path");r.setAttribute("d",e),i.append(r);const o=r.getTotalLength();if(!Number.isFinite(o)||o<=0)return[];const n=Math.max(2,t),s=[];for(let a=0;a<n;a+=1){const l=a/(n-1),c=r.getPointAtLength(o*l);s.push(Fi({x:c.x,y:c.y}))}return s}catch{return[]}}let xt=class extends y{constructor(){super(...arguments),this.label="轨迹",this.helper="",this.image="",this.points=[],this.path="",this._draftPoints=[],this._selectedIndex=-1,this._dragIndex=null}willUpdate(e){if(!e.has("points")&&!e.has("path")||this._dragIndex!==null)return;const i=(Array.isArray(this.points)?this.points:[]).map(Fi),r=i.length>0?i:this.path?Z0(this.path):[],o=r.length>0?r:[];this._draftPoints=o,this._selectedIndex>=o.length&&(this._selectedIndex=o.length-1)}_editorPoint(e){const t=this.renderRoot.querySelector(".canvas");if(!t)return;const i=t.getBoundingClientRect();if(!(i.width<=0||i.height<=0))return Fi({x:(e.clientX-i.left)/i.width*100,y:(e.clientY-i.top)/i.height*100})}_emit(e){const t=e.map(Fi);this._draftPoints=t,this.dispatchEvent(new CustomEvent("points-changed",{bubbles:!0,composed:!0,detail:{points:t,path:Yd(t)}}))}_onCanvasPointerDown(e){if(e.button!==0||e.composedPath().find(r=>r instanceof HTMLElement&&r.dataset&&r.dataset.pointIndex!==void 0))return;const i=this._editorPoint(e);i&&(this._selectedIndex=this._draftPoints.length,this._emit([...this._draftPoints,i]))}_onPointPointerDown(e,t){t.button===0&&(t.preventDefault(),t.stopPropagation(),this._dragIndex=e,this._selectedIndex=e,t.currentTarget.setPointerCapture(t.pointerId))}_onPointPointerMove(e){if(this._dragIndex===null)return;const t=this._editorPoint(e);if(!t)return;const i=this._draftPoints.map((r,o)=>o===this._dragIndex?t:r);this._emit(i)}_onPointPointerUp(){this._dragIndex=null}_removeSelected(){if(this._selectedIndex<0)return;const e=this._draftPoints.filter((t,i)=>i!==this._selectedIndex);this._selectedIndex=Math.min(this._selectedIndex,e.length-1),this._emit(e)}_clear(){this._selectedIndex=-1,this._emit([])}_resetDefault(){this._selectedIndex=Xd.length-1,this._emit(Xd)}render(){const e=this._draftPoints,t=Yd(e),i=this.image||Qo;return h`
      <div class="field">
        <div class="label-row">
          <span class="label">${this.label}</span>
          <div class="actions">
            <button type="button" class="link-btn" @click=${this._resetDefault}>默认轨迹</button>
            <button
              type="button"
              class="link-btn"
              ?disabled=${this._selectedIndex<0}
              @click=${this._removeSelected}
            >
              删除点
            </button>
            <button
              type="button"
              class="link-btn danger"
              ?disabled=${e.length===0}
              @click=${this._clear}
            >
              清空
            </button>
          </div>
        </div>
        <div class="canvas" @pointerdown=${this._onCanvasPointerDown}>
          <img class="background" alt="" src=${i} draggable="false" />
          <svg class="overlay" viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
            ${t?h`
                  <path
                    class="path"
                    d=${t}
                    fill="none"
                    stroke="var(--primary-color)"
                    stroke-width="2.8"
                    stroke-linecap="round"
                    stroke-linejoin="round"
                  ></path>
                `:m}
            ${e.map((r,o)=>h`
                <circle
                  class=${o===this._selectedIndex?"point selected":"point"}
                  cx=${String(r.x)}
                  cy=${String(r.y)}
                  r="2.25"
                ></circle>
              `)}
          </svg>
          ${e.map((r,o)=>h`
              <button
                type="button"
                class=${o===this._selectedIndex?"handle selected":"handle"}
                style=${`left:${r.x}%;top:${r.y}%;`}
                data-point-index=${String(o)}
                @click=${()=>{this._selectedIndex=o}}
                @pointerdown=${n=>this._onPointPointerDown(o,n)}
                @pointermove=${this._onPointPointerMove}
                @pointerup=${this._onPointPointerUp}
                @pointercancel=${this._onPointPointerUp}
                title=${`点 ${o+1}`}
              >
                ${o+1}
              </button>
            `)}
        </div>
        <p class="helper">
          ${this.helper||"点击底图新增轨迹点，拖动点调整位置；卡片会按这些点自动生成轨迹。"}
        </p>
        <p class="meta">
          ${e.length>0?`当前 ${e.length} 个点，轨迹数据：${t}`:"还没有轨迹点，先在底图上点击开始绘制。"}
        </p>
      </div>
    `}};xt.styles=w`
    .field {
      display: flex;
      flex-direction: column;
      gap: 8px;
      margin-bottom: 12px;
    }

    .label-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
    }

    .label {
      font-size: 13px;
      color: var(--secondary-text-color);
    }

    .actions {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
      justify-content: flex-end;
    }

    .link-btn {
      border: none;
      background: transparent;
      color: var(--primary-color);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 0;
    }

    .link-btn.danger {
      color: var(--error-color, #d32f2f);
    }

    .link-btn:disabled {
      cursor: default;
      opacity: 0.45;
    }

    .canvas {
      position: relative;
      width: 100%;
      min-height: 240px;
      border-radius: 12px;
      overflow: hidden;
      border: 1px solid var(--divider-color);
      background: var(--secondary-background-color, #f4f6f8);
      touch-action: none;
      cursor: crosshair;
    }

    .background,
    .overlay {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
    }

    .background {
      object-fit: cover;
      user-select: none;
      pointer-events: none;
    }

    .overlay {
      pointer-events: none;
    }

    .path {
      filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.12));
    }

    .point {
      fill: white;
      stroke: var(--primary-color);
      stroke-width: 0.8;
    }

    .point.selected {
      fill: var(--primary-color);
      stroke: white;
    }

    .handle {
      position: absolute;
      transform: translate(-50%, -50%);
      width: 22px;
      height: 22px;
      border-radius: 999px;
      border: 2px solid var(--primary-color);
      background: white;
      color: var(--primary-color);
      font-size: 11px;
      font-weight: 700;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: grab;
      touch-action: none;
      box-shadow: 0 1px 4px rgba(0, 0, 0, 0.18);
    }

    .handle.selected {
      background: var(--primary-color);
      color: white;
    }

    .handle:active {
      cursor: grabbing;
    }

    .helper,
    .meta {
      margin: 0;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.45;
      word-break: break-word;
    }
  `;ki([u()],xt.prototype,"label",2);ki([u()],xt.prototype,"helper",2);ki([u()],xt.prototype,"image",2);ki([u({attribute:!1})],xt.prototype,"points",2);ki([u()],xt.prototype,"path",2);ki([g()],xt.prototype,"_draftPoints",2);ki([g()],xt.prototype,"_selectedIndex",2);xt=ki([x("flow-path-points-editor")],xt);var K0=Object.defineProperty,Q0=Object.getOwnPropertyDescriptor,ru=(e,t,i,r)=>{for(var o=r>1?void 0:r?Q0(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&K0(t,i,o),o};let Ms=class extends y{constructor(){super(...arguments),this.config={type:"tile"}}_data(){return m0(this.config)}_isVisible(e,t){if(!e.visible)return!0;const i=t[e.visible.field];return e.visible.operator==="eq"?i===e.visible.value:i!==e.visible.value}_emit(e){const t="content_layout"in e,i={...this._data(),...e},r=g0(i,{layoutChanged:t}),o={...this.config,...r};r.text_only||delete o.text_only,Number(r.list_columns)!==2&&delete o.list_columns,this.dispatchEvent(new CustomEvent("config-changed",{bubbles:!0,composed:!0,detail:{config:o}}))}_setField(e,t){e&&this._emit({[e]:t})}async _uploadImage(e,t){const i=t.name.replace(/[^\w.\-]+/g,"_")||"image.png",r=`layout-${Date.now()}-${i}`,o=await fetch(`/api/upload/media?filename=${encodeURIComponent(r)}`,{method:"POST",headers:{"Content-Type":t.type||"application/octet-stream"},body:t});!o.ok||(await o.json()).ok===!1||this._setField(e,`/api/media/download?file=${encodeURIComponent(r)}`)}applyPictureElementPosition(e,t){const i=this.renderRoot.querySelector("flow-picture-elements-editor");return!i||i.selectedIndex<0?!1:(i.applyPosition(e,t),!0)}selectPictureElement(e){const t=this.renderRoot.querySelector("flow-picture-elements-editor");return t?(t.selectIndex(e),!0):!1}_renderField(e,t){var i,r,o,n,s;if(!this._isVisible(e,t))return m;switch(e.type){case"section":return h`
          <ha-expandable-panel .header=${e.label}>
            ${(i=e.schema)==null?void 0:i.map(a=>this._renderField(a,t))}
          </ha-expandable-panel>
        `;case"grid":{const a=e.gridVariant==="appearance";return h`
          <div class="grid ${a?"appearance":""}">
            ${(r=e.schema)==null?void 0:r.map(l=>this._renderField(l,t))}
          </div>
          ${a?h`<p class="appearance-helper">非活动状态（例如关闭或闭合）将不会被着色。</p>`:m}
        `}case"entity":return h`
          <ha-entity-picker
            .label=${e.label}
            .value=${String(t[e.name]??"")}
            .placeholder=${e.placeholder??""}
            ?optional=${e.optional}
            @value-changed=${a=>{const l={[e.name]:a.detail.value};a.detail.actionName&&e.name==="entity"&&(l.name=a.detail.actionName),this._emit(l)}}
          ></ha-entity-picker>
          ${e.helper?h`<p class="helper">${e.helper}</p>`:m}
        `;case"icon":return h`
          <ha-icon-picker
            .label=${e.label}
            .value=${String(t[e.name]??"")}
            .placeholder=${e.placeholder??"mdi:home"}
            ?compact=${e.compact}
            @icon-changed=${a=>this._setField(e.name,a.detail.value)}
          ></ha-icon-picker>
        `;case"color":return h`
          <ha-color-picker
            .label=${e.label}
            .value=${String(t[e.name]??"")}
            ?compact=${e.compact}
            ?hideHelper=${e.compact}
            @color-changed=${a=>this._setField(e.name,a.detail.value)}
          ></ha-color-picker>
        `;case"boolean":return h`
          <label class="switch-row">
            <div>
              <div class="switch-label">${e.label}</div>
              ${e.helper?h`<div class="helper">${e.helper}</div>`:m}
            </div>
            <input
              type="checkbox"
              .checked=${!!t[e.name]}
              @change=${a=>this._setField(e.name,a.target.checked)}
            />
          </label>
        `;case"select":return h`
          <label class="field">
            <span class="label-text">
              ${e.label}
              ${e.optional?h`<span class="optional">（可选）</span>`:m}
            </span>
            <select
              @change=${a=>this._setField(e.name,a.target.value)}
            >
              ${(o=e.options)==null?void 0:o.map(a=>{var c,d;const l=String(t[e.name]??((d=(c=e.options)==null?void 0:c[0])==null?void 0:d.value)??"");return h`
                  <option
                    value=${a.value}
                    ?selected=${l===a.value}
                    title=${a.description??""}
                  >
                    ${a.label}
                  </option>
                `})}
            </select>
            ${e.helper?h`<p class="helper">${e.helper}</p>`:m}
          </label>
        `;case"select_box":{const a=String(t[e.name]??((s=(n=e.options)==null?void 0:n[0])==null?void 0:s.value)??"");return h`
          <ha-select-box
            .label=${e.label}
            .value=${a}
            .options=${e.options??[]}
            .maxColumns=${e.maxColumns??2}
            @value-changed=${l=>{l.stopPropagation(),this._setField(e.name,l.detail.value)}}
          ></ha-select-box>
          ${e.helper?h`<p class="helper">${e.helper}</p>`:m}
        `}case"textarea":return h`
          <label class="field">
            <span class="label-text">${e.label}</span>
            <textarea
              rows="6"
              .value=${String(t[e.name]??"")}
              @input=${a=>this._setField(e.name,a.target.value)}
            ></textarea>
            ${e.helper?h`<p class="helper">${e.helper}</p>`:m}
          </label>
        `;case"number":return h`
          <label class="field">
            <span class="label-text">${e.label}</span>
            <input
              type="number"
              .value=${String(t[e.name]??"")}
              @input=${a=>this._setField(e.name,Number(a.target.value)||0)}
            />
          </label>
        `;case"badges":return h`
          <flow-heading-badges-editor
              .badges=${t.badges??[]}
            @badges-changed=${a=>{a.stopPropagation(),this._setField("badges",a.detail.badges)}}
          ></flow-heading-badges-editor>
        `;case"bar_entities":return h`
          <flow-bar-entities-editor
            .entities=${t.entities??[]}
            @entities-changed=${a=>{a.stopPropagation(),this._setField("entities",a.detail.entities)}}
          ></flow-bar-entities-editor>
        `;case"status_entities":return h`
          <flow-status-entities-editor
            .entities=${t.entities??[]}
            @entities-changed=${a=>{a.stopPropagation(),this._setField("entities",a.detail.entities)}}
          ></flow-status-entities-editor>
        `;case"image":return h`
          <label class="field">
            <span class="label-text">${e.label}</span>
            <input
              type="text"
              .value=${String(t[e.name]??"")}
              placeholder="https://… 或选择本地图片"
              @input=${a=>this._setField(e.name,a.target.value)}
            />
            <div class="image-actions">
              <label class="file-btn">
                上传图片
                <input
                  type="file"
                  accept="image/*"
                  hidden
                  @change=${a=>{var c;const l=(c=a.target.files)==null?void 0:c[0];a.target.value="",l&&this._uploadImage(e.name,l)}}
                />
              </label>
              <button
                type="button"
                class="link-btn"
                @click=${()=>this._setField(e.name,Qo)}
              >
                使用默认户型图
              </button>
            </div>
            ${e.helper?h`<p class="helper">${e.helper}</p>`:m}
          </label>
        `;case"picture_elements":return h`
          <flow-picture-elements-editor
            .elements=${t.elements??[]}
            @elements-changed=${a=>{a.stopPropagation(),this._setField("elements",a.detail.elements)}}
          ></flow-picture-elements-editor>
        `;case"path_points":return h`
          <flow-path-points-editor
            .label=${e.label}
            .helper=${e.helper??""}
            .image=${String(t.image??Qo)}
            .points=${t.points??[]}
            .path=${String(t.path??"")}
            @points-changed=${a=>{a.stopPropagation(),this._emit({points:a.detail.points,path:a.detail.path})}}
          ></flow-path-points-editor>
        `;case"path_badges":return h`
          <flow-path-badges-editor
            .badges=${t.badges??[]}
            @badges-changed=${a=>{a.stopPropagation(),this._setField("badges",a.detail.badges)}}
          ></flow-path-badges-editor>
        `;case"text":default:return h`
          <label class="field">
            <span class="label-text">
              ${e.label}
              ${e.optional?h`<span class="optional">（可选）</span>`:m}
            </span>
            <input
              type="text"
              .value=${String(t[e.name]??"")}
              placeholder=${e.placeholder??""}
              @input=${a=>this._setField(e.name,a.target.value)}
            />
            ${e.helper?h`<p class="helper">${e.helper}</p>`:m}
          </label>
        `}}render(){const e=String(this.config.type??"tile"),t=u0(e),i=this._data();return h`
      <div class="config-editor">
        <label class="field type-field">
          <span class="label-text">类型</span>
          <input type="text" .value=${e} disabled />
        </label>
        ${t.map(r=>this._renderField(r,i))}
      </div>
    `}};Ms.styles=w`
    .config-editor {
      display: flex;
      flex-direction: column;
    }
    .type-field input:disabled {
      opacity: 0.7;
      background: var(--secondary-background-color, #f5f5f5);
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
    }
    .label-text {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .optional {
      font-weight: 400;
      opacity: 0.8;
    }
    input,
    textarea,
    select {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    textarea {
      resize: vertical;
      min-height: 120px;
      font-family: inherit;
      line-height: 1.5;
    }
    .helper {
      margin: -4px 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 16px;
      align-items: start;
      margin-bottom: 12px;
    }
    .grid.appearance {
      margin-bottom: 0;
    }
    .grid > * {
      min-width: 0;
    }
    .appearance-helper {
      margin: 4px 0 12px;
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
    @media (max-width: 520px) {
      .grid {
        grid-template-columns: 1fr;
      }
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 12px;
      cursor: pointer;
    }
    .switch-label {
      font-size: 14px;
      font-weight: 500;
      color: var(--primary-text-color);
    }
    .switch-row input[type="checkbox"] {
      width: 18px;
      height: 18px;
      accent-color: var(--primary-color);
    }
    .image-actions {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-top: 2px;
    }
    .file-btn,
    .link-btn {
      border: none;
      background: transparent;
      color: var(--primary-color);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 0;
    }
    .file-btn {
      display: inline-flex;
    }
  `;ru([u({attribute:!1})],Ms.prototype,"config",2);Ms=ru([x("flow-card-config-editor")],Ms);/*! Hammer.JS - v2.0.17-rc - 2019-12-16
 * http://naver.github.io/egjs
 *
 * Forked By Naver egjs
 * Copyright (c) hammerjs
 * Licensed under the MIT license */function it(){return it=Object.assign||function(e){for(var t=1;t<arguments.length;t++){var i=arguments[t];for(var r in i)Object.prototype.hasOwnProperty.call(i,r)&&(e[r]=i[r])}return e},it.apply(this,arguments)}function We(e,t){e.prototype=Object.create(t.prototype),e.prototype.constructor=e,e.__proto__=t}function jn(e){if(e===void 0)throw new ReferenceError("this hasn't been initialised - super() hasn't been called");return e}var pl;typeof Object.assign!="function"?pl=function(t){if(t==null)throw new TypeError("Cannot convert undefined or null to object");for(var i=Object(t),r=1;r<arguments.length;r++){var o=arguments[r];if(o!=null)for(var n in o)o.hasOwnProperty(n)&&(i[n]=o[n])}return i}:pl=Object.assign;var rr=pl,Zd=["","webkit","Moz","MS","ms","o"],J0=typeof document>"u"?{style:{}}:document.createElement("div"),ey="function",Ar=Math.round,or=Math.abs,mc=Date.now;function ta(e,t){for(var i,r,o=t[0].toUpperCase()+t.slice(1),n=0;n<Zd.length;){if(i=Zd[n],r=i?i+o:t,r in e)return r;n++}}var It;typeof window>"u"?It={}:It=window;var ou=ta(J0.style,"touchAction"),nu=ou!==void 0;function ty(){if(!nu)return!1;var e={},t=It.CSS&&It.CSS.supports;return["auto","manipulation","pan-y","pan-x","pan-x pan-y","none"].forEach(function(i){return e[i]=t?It.CSS.supports("touch-action",i):!0}),e}var su="compute",au="auto",ul="manipulation",Bi="none",nn="pan-x",sn="pan-y",qn=ty(),iy=/mobile|tablet|ip(ad|hone|od)|android/i,lu="ontouchstart"in It,ry=ta(It,"PointerEvent")!==void 0,oy=lu&&iy.test(navigator.userAgent),$n="touch",ny="pen",bc="mouse",sy="kinect",ay=25,fe=1,br=2,ie=4,Ce=8,Os=1,kn=2,Cn=4,Sn=8,eo=16,ut=kn|Cn,Ui=Sn|eo,vc=ut|Ui,cu=["x","y"],Ls=["clientX","clientY"];function Ht(e,t,i){var r;if(e)if(e.forEach)e.forEach(t,i);else if(e.length!==void 0)for(r=0;r<e.length;)t.call(i,e[r],r,e),r++;else for(r in e)e.hasOwnProperty(r)&&t.call(i,e[r],r,e)}function ia(e,t){return typeof e===ey?e.apply(t&&t[0]||void 0,t):e}function zi(e,t){return e.indexOf(t)>-1}function ly(e){if(zi(e,Bi))return Bi;var t=zi(e,nn),i=zi(e,sn);return t&&i?Bi:t||i?t?nn:sn:zi(e,ul)?ul:au}var du=function(){function e(i,r){this.manager=i,this.set(r)}var t=e.prototype;return t.set=function(r){r===su&&(r=this.compute()),nu&&this.manager.element.style&&qn[r]&&(this.manager.element.style[ou]=r),this.actions=r.toLowerCase().trim()},t.update=function(){this.set(this.manager.options.touchAction)},t.compute=function(){var r=[];return Ht(this.manager.recognizers,function(o){ia(o.options.enable,[o])&&(r=r.concat(o.getTouchAction()))}),ly(r.join(" "))},t.preventDefaults=function(r){var o=r.srcEvent,n=r.offsetDirection;if(this.manager.session.prevented){o.preventDefault();return}var s=this.actions,a=zi(s,Bi)&&!qn[Bi],l=zi(s,sn)&&!qn[sn],c=zi(s,nn)&&!qn[nn];if(a){var d=r.pointers.length===1,p=r.distance<2,f=r.deltaTime<250;if(d&&p&&f)return}if(!(c&&l)&&(a||l&&n&ut||c&&n&Ui))return this.preventSrc(o)},t.preventSrc=function(r){this.manager.session.prevented=!0,r.preventDefault()},e}();function _c(e,t){for(;e;){if(e===t)return!0;e=e.parentNode}return!1}function hu(e){var t=e.length;if(t===1)return{x:Ar(e[0].clientX),y:Ar(e[0].clientY)};for(var i=0,r=0,o=0;o<t;)i+=e[o].clientX,r+=e[o].clientY,o++;return{x:Ar(i/t),y:Ar(r/t)}}function Kd(e){for(var t=[],i=0;i<e.pointers.length;)t[i]={clientX:Ar(e.pointers[i].clientX),clientY:Ar(e.pointers[i].clientY)},i++;return{timeStamp:mc(),pointers:t,center:hu(t),deltaX:e.deltaX,deltaY:e.deltaY}}function Ds(e,t,i){i||(i=cu);var r=t[i[0]]-e[i[0]],o=t[i[1]]-e[i[1]];return Math.sqrt(r*r+o*o)}function fl(e,t,i){i||(i=cu);var r=t[i[0]]-e[i[0]],o=t[i[1]]-e[i[1]];return Math.atan2(o,r)*180/Math.PI}function pu(e,t){return e===t?Os:or(e)>=or(t)?e<0?kn:Cn:t<0?Sn:eo}function cy(e,t){var i=t.center,r=e.offsetDelta||{},o=e.prevDelta||{},n=e.prevInput||{};(t.eventType===fe||n.eventType===ie)&&(o=e.prevDelta={x:n.deltaX||0,y:n.deltaY||0},r=e.offsetDelta={x:i.x,y:i.y}),t.deltaX=o.x+(i.x-r.x),t.deltaY=o.y+(i.y-r.y)}function uu(e,t,i){return{x:t/e||0,y:i/e||0}}function dy(e,t){return Ds(t[0],t[1],Ls)/Ds(e[0],e[1],Ls)}function hy(e,t){return fl(t[1],t[0],Ls)+fl(e[1],e[0],Ls)}function py(e,t){var i=e.lastInterval||t,r=t.timeStamp-i.timeStamp,o,n,s,a;if(t.eventType!==Ce&&(r>ay||i.velocity===void 0)){var l=t.deltaX-i.deltaX,c=t.deltaY-i.deltaY,d=uu(r,l,c);n=d.x,s=d.y,o=or(d.x)>or(d.y)?d.x:d.y,a=pu(l,c),e.lastInterval=t}else o=i.velocity,n=i.velocityX,s=i.velocityY,a=i.direction;t.velocity=o,t.velocityX=n,t.velocityY=s,t.direction=a}function uy(e,t){var i=e.session,r=t.pointers,o=r.length;i.firstInput||(i.firstInput=Kd(t)),o>1&&!i.firstMultiple?i.firstMultiple=Kd(t):o===1&&(i.firstMultiple=!1);var n=i.firstInput,s=i.firstMultiple,a=s?s.center:n.center,l=t.center=hu(r);t.timeStamp=mc(),t.deltaTime=t.timeStamp-n.timeStamp,t.angle=fl(a,l),t.distance=Ds(a,l),cy(i,t),t.offsetDirection=pu(t.deltaX,t.deltaY);var c=uu(t.deltaTime,t.deltaX,t.deltaY);t.overallVelocityX=c.x,t.overallVelocityY=c.y,t.overallVelocity=or(c.x)>or(c.y)?c.x:c.y,t.scale=s?dy(s.pointers,r):1,t.rotation=s?hy(s.pointers,r):0,t.maxPointers=i.prevInput?t.pointers.length>i.prevInput.maxPointers?t.pointers.length:i.prevInput.maxPointers:t.pointers.length,py(i,t);var d=e.element,p=t.srcEvent,f;p.composedPath?f=p.composedPath()[0]:p.path?f=p.path[0]:f=p.target,_c(f,d)&&(d=f),t.target=d}function fy(e,t,i){var r=i.pointers.length,o=i.changedPointers.length,n=t&fe&&r-o===0,s=t&(ie|Ce)&&r-o===0;i.isFirst=!!n,i.isFinal=!!s,n&&(e.session={}),i.eventType=t,uy(e,i),e.emit("hammer.input",i),e.recognize(i),e.session.prevInput=i}function an(e){return e.trim().split(/\s+/g)}function Ro(e,t,i){Ht(an(t),function(r){e.addEventListener(r,i,!1)})}function Fo(e,t,i){Ht(an(t),function(r){e.removeEventListener(r,i,!1)})}function Qd(e){var t=e.ownerDocument||e;return t.defaultView||t.parentWindow||window}var ho=function(){function e(i,r){var o=this;this.manager=i,this.callback=r,this.element=i.element,this.target=i.options.inputTarget,this.domHandler=function(n){ia(i.options.enable,[i])&&o.handler(n)},this.init()}var t=e.prototype;return t.handler=function(){},t.init=function(){this.evEl&&Ro(this.element,this.evEl,this.domHandler),this.evTarget&&Ro(this.target,this.evTarget,this.domHandler),this.evWin&&Ro(Qd(this.element),this.evWin,this.domHandler)},t.destroy=function(){this.evEl&&Fo(this.element,this.evEl,this.domHandler),this.evTarget&&Fo(this.target,this.evTarget,this.domHandler),this.evWin&&Fo(Qd(this.element),this.evWin,this.domHandler)},e}();function nr(e,t,i){if(e.indexOf&&!i)return e.indexOf(t);for(var r=0;r<e.length;){if(i&&e[r][i]==t||!i&&e[r]===t)return r;r++}return-1}var gy={pointerdown:fe,pointermove:br,pointerup:ie,pointercancel:Ce,pointerout:Ce},my={2:$n,3:ny,4:bc,5:sy},fu="pointerdown",gu="pointermove pointerup pointercancel";It.MSPointerEvent&&!It.PointerEvent&&(fu="MSPointerDown",gu="MSPointerMove MSPointerUp MSPointerCancel");var mu=function(e){We(t,e);function t(){var r,o=t.prototype;return o.evEl=fu,o.evWin=gu,r=e.apply(this,arguments)||this,r.store=r.manager.session.pointerEvents=[],r}var i=t.prototype;return i.handler=function(o){var n=this.store,s=!1,a=o.type.toLowerCase().replace("ms",""),l=gy[a],c=my[o.pointerType]||o.pointerType,d=c===$n,p=nr(n,o.pointerId,"pointerId");l&fe&&(o.button===0||d)?p<0&&(n.push(o),p=n.length-1):l&(ie|Ce)&&(s=!0),!(p<0)&&(n[p]=o,this.callback(this.manager,l,{pointers:n,changedPointers:[o],pointerType:c,srcEvent:o}),s&&n.splice(p,1))},t}(ho);function ln(e){return Array.prototype.slice.call(e,0)}function yc(e,t,i){for(var r=[],o=[],n=0;n<e.length;){var s=t?e[n][t]:e[n];nr(o,s)<0&&r.push(e[n]),o[n]=s,n++}return i&&(t?r=r.sort(function(a,l){return a[t]>l[t]}):r=r.sort()),r}var by={touchstart:fe,touchmove:br,touchend:ie,touchcancel:Ce},vy="touchstart touchmove touchend touchcancel",xc=function(e){We(t,e);function t(){var r;return t.prototype.evTarget=vy,r=e.apply(this,arguments)||this,r.targetIds={},r}var i=t.prototype;return i.handler=function(o){var n=by[o.type],s=_y.call(this,o,n);s&&this.callback(this.manager,n,{pointers:s[0],changedPointers:s[1],pointerType:$n,srcEvent:o})},t}(ho);function _y(e,t){var i=ln(e.touches),r=this.targetIds;if(t&(fe|br)&&i.length===1)return r[i[0].identifier]=!0,[i,i];var o,n,s=ln(e.changedTouches),a=[],l=this.target;if(n=i.filter(function(c){return _c(c.target,l)}),t===fe)for(o=0;o<n.length;)r[n[o].identifier]=!0,o++;for(o=0;o<s.length;)r[s[o].identifier]&&a.push(s[o]),t&(ie|Ce)&&delete r[s[o].identifier],o++;if(a.length)return[yc(n.concat(a),"identifier",!0),a]}var yy={mousedown:fe,mousemove:br,mouseup:ie},xy="mousedown",wy="mousemove mouseup",wc=function(e){We(t,e);function t(){var r,o=t.prototype;return o.evEl=xy,o.evWin=wy,r=e.apply(this,arguments)||this,r.pressed=!1,r}var i=t.prototype;return i.handler=function(o){var n=yy[o.type];n&fe&&o.button===0&&(this.pressed=!0),n&br&&o.which!==1&&(n=ie),this.pressed&&(n&ie&&(this.pressed=!1),this.callback(this.manager,n,{pointers:[o],changedPointers:[o],pointerType:bc,srcEvent:o}))},t}(ho),$y=2500,Jd=25;function eh(e){var t=e.changedPointers,i=t[0];if(i.identifier===this.primaryTouch){var r={x:i.clientX,y:i.clientY},o=this.lastTouches;this.lastTouches.push(r);var n=function(){var a=o.indexOf(r);a>-1&&o.splice(a,1)};setTimeout(n,$y)}}function ky(e,t){e&fe?(this.primaryTouch=t.changedPointers[0].identifier,eh.call(this,t)):e&(ie|Ce)&&eh.call(this,t)}function Cy(e){for(var t=e.srcEvent.clientX,i=e.srcEvent.clientY,r=0;r<this.lastTouches.length;r++){var o=this.lastTouches[r],n=Math.abs(t-o.x),s=Math.abs(i-o.y);if(n<=Jd&&s<=Jd)return!0}return!1}var bu=function(){var e=function(t){We(i,t);function i(o,n){var s;return s=t.call(this,o,n)||this,s.handler=function(a,l,c){var d=c.pointerType===$n,p=c.pointerType===bc;if(!(p&&c.sourceCapabilities&&c.sourceCapabilities.firesTouchEvents)){if(d)ky.call(jn(jn(s)),l,c);else if(p&&Cy.call(jn(jn(s)),c))return;s.callback(a,l,c)}},s.touch=new xc(s.manager,s.handler),s.mouse=new wc(s.manager,s.handler),s.primaryTouch=null,s.lastTouches=[],s}var r=i.prototype;return r.destroy=function(){this.touch.destroy(),this.mouse.destroy()},i}(ho);return e}();function Sy(e){var t,i=e.options.inputClass;return i?t=i:ry?t=mu:oy?t=xc:lu?t=bu:t=wc,new t(e,fy)}function Tr(e,t,i){return Array.isArray(e)?(Ht(e,i[t],i),!0):!1}var as=1,Ne=2,to=4,ci=8,zt=ci,cn=16,ht=32,Ey=1;function Py(){return Ey++}function Gn(e,t){var i=t.manager;return i?i.get(e):e}function th(e){return e&cn?"cancel":e&ci?"end":e&to?"move":e&Ne?"start":""}var En=function(){function e(i){i===void 0&&(i={}),this.options=it({enable:!0},i),this.id=Py(),this.manager=null,this.state=as,this.simultaneous={},this.requireFail=[]}var t=e.prototype;return t.set=function(r){return rr(this.options,r),this.manager&&this.manager.touchAction.update(),this},t.recognizeWith=function(r){if(Tr(r,"recognizeWith",this))return this;var o=this.simultaneous;return r=Gn(r,this),o[r.id]||(o[r.id]=r,r.recognizeWith(this)),this},t.dropRecognizeWith=function(r){return Tr(r,"dropRecognizeWith",this)?this:(r=Gn(r,this),delete this.simultaneous[r.id],this)},t.requireFailure=function(r){if(Tr(r,"requireFailure",this))return this;var o=this.requireFail;return r=Gn(r,this),nr(o,r)===-1&&(o.push(r),r.requireFailure(this)),this},t.dropRequireFailure=function(r){if(Tr(r,"dropRequireFailure",this))return this;r=Gn(r,this);var o=nr(this.requireFail,r);return o>-1&&this.requireFail.splice(o,1),this},t.hasRequireFailures=function(){return this.requireFail.length>0},t.canRecognizeWith=function(r){return!!this.simultaneous[r.id]},t.emit=function(r){var o=this,n=this.state;function s(a){o.manager.emit(a,r)}n<ci&&s(o.options.event+th(n)),s(o.options.event),r.additionalEvent&&s(r.additionalEvent),n>=ci&&s(o.options.event+th(n))},t.tryEmit=function(r){if(this.canEmit())return this.emit(r);this.state=ht},t.canEmit=function(){for(var r=0;r<this.requireFail.length;){if(!(this.requireFail[r].state&(ht|as)))return!1;r++}return!0},t.recognize=function(r){var o=rr({},r);if(!ia(this.options.enable,[this,o])){this.reset(),this.state=ht;return}this.state&(zt|cn|ht)&&(this.state=as),this.state=this.process(o),this.state&(Ne|to|ci|cn)&&this.tryEmit(o)},t.process=function(r){},t.getTouchAction=function(){},t.reset=function(){},e}(),Is=function(e){We(t,e);function t(r){var o;return r===void 0&&(r={}),o=e.call(this,it({event:"tap",pointers:1,taps:1,interval:300,time:250,threshold:9,posThreshold:10},r))||this,o.pTime=!1,o.pCenter=!1,o._timer=null,o._input=null,o.count=0,o}var i=t.prototype;return i.getTouchAction=function(){return[ul]},i.process=function(o){var n=this,s=this.options,a=o.pointers.length===s.pointers,l=o.distance<s.threshold,c=o.deltaTime<s.time;if(this.reset(),o.eventType&fe&&this.count===0)return this.failTimeout();if(l&&c&&a){if(o.eventType!==ie)return this.failTimeout();var d=this.pTime?o.timeStamp-this.pTime<s.interval:!0,p=!this.pCenter||Ds(this.pCenter,o.center)<s.posThreshold;this.pTime=o.timeStamp,this.pCenter=o.center,!p||!d?this.count=1:this.count+=1,this._input=o;var f=this.count%s.taps;if(f===0)return this.hasRequireFailures()?(this._timer=setTimeout(function(){n.state=zt,n.tryEmit()},s.interval),Ne):zt}return ht},i.failTimeout=function(){var o=this;return this._timer=setTimeout(function(){o.state=ht},this.options.interval),ht},i.reset=function(){clearTimeout(this._timer)},i.emit=function(){this.state===zt&&(this._input.tapCount=this.count,this.manager.emit(this.options.event,this._input))},t}(En),io=function(e){We(t,e);function t(r){return r===void 0&&(r={}),e.call(this,it({pointers:1},r))||this}var i=t.prototype;return i.attrTest=function(o){var n=this.options.pointers;return n===0||o.pointers.length===n},i.process=function(o){var n=this.state,s=o.eventType,a=n&(Ne|to),l=this.attrTest(o);return a&&(s&Ce||!l)?n|cn:a||l?s&ie?n|ci:n&Ne?n|to:Ne:ht},t}(En);function vu(e){return e===eo?"down":e===Sn?"up":e===kn?"left":e===Cn?"right":""}var ra=function(e){We(t,e);function t(r){var o;return r===void 0&&(r={}),o=e.call(this,it({event:"pan",threshold:10,pointers:1,direction:vc},r))||this,o.pX=null,o.pY=null,o}var i=t.prototype;return i.getTouchAction=function(){var o=this.options.direction,n=[];return o&ut&&n.push(sn),o&Ui&&n.push(nn),n},i.directionTest=function(o){var n=this.options,s=!0,a=o.distance,l=o.direction,c=o.deltaX,d=o.deltaY;return l&n.direction||(n.direction&ut?(l=c===0?Os:c<0?kn:Cn,s=c!==this.pX,a=Math.abs(o.deltaX)):(l=d===0?Os:d<0?Sn:eo,s=d!==this.pY,a=Math.abs(o.deltaY))),o.direction=l,s&&a>n.threshold&&l&n.direction},i.attrTest=function(o){return io.prototype.attrTest.call(this,o)&&(this.state&Ne||!(this.state&Ne)&&this.directionTest(o))},i.emit=function(o){this.pX=o.deltaX,this.pY=o.deltaY;var n=vu(o.direction);n&&(o.additionalEvent=this.options.event+n),e.prototype.emit.call(this,o)},t}(io),_u=function(e){We(t,e);function t(r){return r===void 0&&(r={}),e.call(this,it({event:"swipe",threshold:10,velocity:.3,direction:ut|Ui,pointers:1},r))||this}var i=t.prototype;return i.getTouchAction=function(){return ra.prototype.getTouchAction.call(this)},i.attrTest=function(o){var n=this.options.direction,s;return n&(ut|Ui)?s=o.overallVelocity:n&ut?s=o.overallVelocityX:n&Ui&&(s=o.overallVelocityY),e.prototype.attrTest.call(this,o)&&n&o.offsetDirection&&o.distance>this.options.threshold&&o.maxPointers===this.options.pointers&&or(s)>this.options.velocity&&o.eventType&ie},i.emit=function(o){var n=vu(o.offsetDirection);n&&this.manager.emit(this.options.event+n,o),this.manager.emit(this.options.event,o)},t}(io),yu=function(e){We(t,e);function t(r){return r===void 0&&(r={}),e.call(this,it({event:"pinch",threshold:0,pointers:2},r))||this}var i=t.prototype;return i.getTouchAction=function(){return[Bi]},i.attrTest=function(o){return e.prototype.attrTest.call(this,o)&&(Math.abs(o.scale-1)>this.options.threshold||this.state&Ne)},i.emit=function(o){if(o.scale!==1){var n=o.scale<1?"in":"out";o.additionalEvent=this.options.event+n}e.prototype.emit.call(this,o)},t}(io),xu=function(e){We(t,e);function t(r){return r===void 0&&(r={}),e.call(this,it({event:"rotate",threshold:0,pointers:2},r))||this}var i=t.prototype;return i.getTouchAction=function(){return[Bi]},i.attrTest=function(o){return e.prototype.attrTest.call(this,o)&&(Math.abs(o.rotation)>this.options.threshold||this.state&Ne)},t}(io),wu=function(e){We(t,e);function t(r){var o;return r===void 0&&(r={}),o=e.call(this,it({event:"press",pointers:1,time:251,threshold:9},r))||this,o._timer=null,o._input=null,o}var i=t.prototype;return i.getTouchAction=function(){return[au]},i.process=function(o){var n=this,s=this.options,a=o.pointers.length===s.pointers,l=o.distance<s.threshold,c=o.deltaTime>s.time;if(this._input=o,!l||!a||o.eventType&(ie|Ce)&&!c)this.reset();else if(o.eventType&fe)this.reset(),this._timer=setTimeout(function(){n.state=zt,n.tryEmit()},s.time);else if(o.eventType&ie)return zt;return ht},i.reset=function(){clearTimeout(this._timer)},i.emit=function(o){this.state===zt&&(o&&o.eventType&ie?this.manager.emit(this.options.event+"up",o):(this._input.timeStamp=mc(),this.manager.emit(this.options.event,this._input)))},t}(En),$u={domEvents:!1,touchAction:su,enable:!0,inputTarget:null,inputClass:null,cssProps:{userSelect:"none",touchSelect:"none",touchCallout:"none",contentZooming:"none",userDrag:"none",tapHighlightColor:"rgba(0,0,0,0)"}},ih=[[xu,{enable:!1}],[yu,{enable:!1},["rotate"]],[_u,{direction:ut}],[ra,{direction:ut},["swipe"]],[Is],[Is,{event:"doubletap",taps:2},["tap"]],[wu]],Ay=1,rh=2;function oh(e,t){var i=e.element;if(i.style){var r;Ht(e.options.cssProps,function(o,n){r=ta(i.style,n),t?(e.oldCssProps[r]=i.style[r],i.style[r]=o):i.style[r]=e.oldCssProps[r]||""}),t||(e.oldCssProps={})}}function Ty(e,t){var i=document.createEvent("Event");i.initEvent(e,!0,!0),i.gesture=t,t.target.dispatchEvent(i)}var gl=function(){function e(i,r){var o=this;this.options=rr({},$u,r||{}),this.options.inputTarget=this.options.inputTarget||i,this.handlers={},this.session={},this.recognizers=[],this.oldCssProps={},this.element=i,this.input=Sy(this),this.touchAction=new du(this,this.options.touchAction),oh(this,!0),Ht(this.options.recognizers,function(n){var s=o.add(new n[0](n[1]));n[2]&&s.recognizeWith(n[2]),n[3]&&s.requireFailure(n[3])},this)}var t=e.prototype;return t.set=function(r){return rr(this.options,r),r.touchAction&&this.touchAction.update(),r.inputTarget&&(this.input.destroy(),this.input.target=r.inputTarget,this.input.init()),this},t.stop=function(r){this.session.stopped=r?rh:Ay},t.recognize=function(r){var o=this.session;if(!o.stopped){this.touchAction.preventDefaults(r);var n,s=this.recognizers,a=o.curRecognizer;(!a||a&&a.state&zt)&&(o.curRecognizer=null,a=null);for(var l=0;l<s.length;)n=s[l],o.stopped!==rh&&(!a||n===a||n.canRecognizeWith(a))?n.recognize(r):n.reset(),!a&&n.state&(Ne|to|ci)&&(o.curRecognizer=n,a=n),l++}},t.get=function(r){if(r instanceof En)return r;for(var o=this.recognizers,n=0;n<o.length;n++)if(o[n].options.event===r)return o[n];return null},t.add=function(r){if(Tr(r,"add",this))return this;var o=this.get(r.options.event);return o&&this.remove(o),this.recognizers.push(r),r.manager=this,this.touchAction.update(),r},t.remove=function(r){if(Tr(r,"remove",this))return this;var o=this.get(r);if(r){var n=this.recognizers,s=nr(n,o);s!==-1&&(n.splice(s,1),this.touchAction.update())}return this},t.on=function(r,o){if(r===void 0||o===void 0)return this;var n=this.handlers;return Ht(an(r),function(s){n[s]=n[s]||[],n[s].push(o)}),this},t.off=function(r,o){if(r===void 0)return this;var n=this.handlers;return Ht(an(r),function(s){o?n[s]&&n[s].splice(nr(n[s],o),1):delete n[s]}),this},t.emit=function(r,o){this.options.domEvents&&Ty(r,o);var n=this.handlers[r]&&this.handlers[r].slice();if(!(!n||!n.length)){o.type=r,o.preventDefault=function(){o.srcEvent.preventDefault()};for(var s=0;s<n.length;)n[s](o),s++}},t.destroy=function(){this.element&&oh(this,!1),this.handlers={},this.session={},this.input.destroy(),this.element=null},e}(),My={touchstart:fe,touchmove:br,touchend:ie,touchcancel:Ce},Oy="touchstart",Ly="touchstart touchmove touchend touchcancel",Dy=function(e){We(t,e);function t(){var r,o=t.prototype;return o.evTarget=Oy,o.evWin=Ly,r=e.apply(this,arguments)||this,r.started=!1,r}var i=t.prototype;return i.handler=function(o){var n=My[o.type];if(n===fe&&(this.started=!0),!!this.started){var s=Iy.call(this,o,n);n&(ie|Ce)&&s[0].length-s[1].length===0&&(this.started=!1),this.callback(this.manager,n,{pointers:s[0],changedPointers:s[1],pointerType:$n,srcEvent:o})}},t}(ho);function Iy(e,t){var i=ln(e.touches),r=ln(e.changedTouches);return t&(ie|Ce)&&(i=yc(i.concat(r),"identifier",!0)),[i,r]}function ku(e,t,i){var r="DEPRECATED METHOD: "+t+`
`+i+` AT 
`;return function(){var o=new Error("get-stack-trace"),n=o&&o.stack?o.stack.replace(/^[^\(]+?[\n$]/gm,"").replace(/^\s+at\s+/gm,"").replace(/^Object.<anonymous>\s*\(/gm,"{anonymous}()@"):"Unknown Stack Trace",s=window.console&&(window.console.warn||window.console.log);return s&&s.call(window.console,r,n),e.apply(this,arguments)}}var Cu=ku(function(e,t,i){for(var r=Object.keys(t),o=0;o<r.length;)(!i||i&&e[r[o]]===void 0)&&(e[r[o]]=t[r[o]]),o++;return e},"extend","Use `assign`."),Hy=ku(function(e,t){return Cu(e,t,!0)},"merge","Use `assign`.");function zy(e,t,i){var r=t.prototype,o;o=e.prototype=Object.create(r),o.constructor=e,o._super=r,i&&rr(o,i)}function nh(e,t){return function(){return e.apply(t,arguments)}}var Vy=function(){var e=function(i,r){return r===void 0&&(r={}),new gl(i,it({recognizers:ih.concat()},r))};return e.VERSION="2.0.17-rc",e.DIRECTION_ALL=vc,e.DIRECTION_DOWN=eo,e.DIRECTION_LEFT=kn,e.DIRECTION_RIGHT=Cn,e.DIRECTION_UP=Sn,e.DIRECTION_HORIZONTAL=ut,e.DIRECTION_VERTICAL=Ui,e.DIRECTION_NONE=Os,e.DIRECTION_DOWN=eo,e.INPUT_START=fe,e.INPUT_MOVE=br,e.INPUT_END=ie,e.INPUT_CANCEL=Ce,e.STATE_POSSIBLE=as,e.STATE_BEGAN=Ne,e.STATE_CHANGED=to,e.STATE_ENDED=ci,e.STATE_RECOGNIZED=zt,e.STATE_CANCELLED=cn,e.STATE_FAILED=ht,e.Manager=gl,e.Input=ho,e.TouchAction=du,e.TouchInput=xc,e.MouseInput=wc,e.PointerEventInput=mu,e.TouchMouseInput=bu,e.SingleTouchInput=Dy,e.Recognizer=En,e.AttrRecognizer=io,e.Tap=Is,e.Pan=ra,e.Swipe=_u,e.Pinch=yu,e.Rotate=xu,e.Press=wu,e.on=Ro,e.off=Fo,e.each=Ht,e.merge=Hy,e.extend=Cu,e.bindFn=nh,e.assign=rr,e.inherit=zy,e.bindFn=nh,e.prefixed=ta,e.toArray=ln,e.inArray=nr,e.uniqueArray=yc,e.splitStr=an,e.boolOrFn=ia,e.hasParent=_c,e.addEventListeners=Ro,e.removeEventListeners=Fo,e.defaults=rr({},$u,{preset:ih}),e}();Vy.defaults;var Ny=Object.defineProperty,Ry=Object.getOwnPropertyDescriptor,De=(e,t,i,r)=>{for(var o=r>1?void 0:r?Ry(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ny(t,i,o),o};let ge=class extends y{constructor(){super(...arguments),this.disabled=!1,this.vertical=!1,this.tooltipMode="interaction",this.step=1,this.min=1,this.max=4,this.pressed=!1,this.tooltipVisible=!1,this._handleKeyDown=e=>{if(new Set(["ArrowRight","ArrowUp","ArrowLeft","ArrowDown","PageUp","PageDown","Home","End"]).has(e.code)){switch(e.preventDefault(),e.code){case"ArrowRight":case"ArrowUp":this.value=this._boundedValue((this.value??0)+this.step);break;case"ArrowLeft":case"ArrowDown":this.value=this._boundedValue((this.value??0)-this.step);break;case"PageUp":this.value=this._steppedValue(this._boundedValue((this.value??0)+this._tenPercentStep));break;case"PageDown":this.value=this._steppedValue(this._boundedValue((this.value??0)-this._tenPercentStep));break;case"Home":this.value=this.min;break;case"End":this.value=this.max;break}this.dispatchEvent(new CustomEvent("slider-moved",{bubbles:!0,composed:!0,detail:{value:this.value}}))}},this._handleKeyUp=e=>{new Set(["ArrowRight","ArrowUp","ArrowLeft","ArrowDown","PageUp","PageDown","Home","End"]).has(e.code)&&(e.preventDefault(),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:this.value}})))},this._getPercentageFromEvent=e=>{if(this.vertical){const o=e.center.y,n=e.target.getBoundingClientRect().top,s=e.target.clientHeight;return Math.max(Math.min(1,(o-n)/s),0)}const t=e.center.x,i=e.target.getBoundingClientRect().left,r=e.target.clientWidth;return Math.max(Math.min(1,(t-i)/r),0)}}get _range(){return this.range??this.max}_valueToPercentage(e){return this._boundedValue(e)/this._range}_percentageToValue(e){return this._range*e}_steppedValue(e){return Math.round(e/this.step)*this.step}_boundedValue(e){return Math.min(Math.max(e,this.min),this.max)}firstUpdated(e){super.firstUpdated(e),this.setupListeners(),this.setAttribute("role","slider"),this.hasAttribute("tabindex")||this.setAttribute("tabindex","0")}updated(e){if(super.updated(e),e.has("value")){const t=this._steppedValue(this.value??0);this.setAttribute("aria-valuenow",t.toString()),this.setAttribute("aria-valuetext",t.toString())}e.has("min")&&this.setAttribute("aria-valuemin",this.min.toString()),e.has("max")&&this.setAttribute("aria-valuemax",this.max.toString()),e.has("vertical")&&this.setAttribute("aria-orientation",this.vertical?"vertical":"horizontal")}connectedCallback(){super.connectedCallback(),this.setupListeners()}disconnectedCallback(){super.disconnectedCallback(),this.destroyListeners()}setupListeners(){if(this.slider&&!this._mc){this._mc=new gl(this.slider,{touchAction:this.touchAction??(this.vertical?"pan-x":"pan-y")}),this._mc.add(new ra({threshold:10,direction:vc,enable:!0})),this._mc.add(new Is({event:"singletap"}));let e;this._mc.on("panstart",()=>{this.disabled||(this.pressed=!0,this._showTooltip(),e=this.value)}),this._mc.on("pancancel",()=>{this.disabled||(this.pressed=!1,this._hideTooltip(),this.value=e)}),this._mc.on("panmove",t=>{if(this.disabled)return;const i=this._getPercentageFromEvent(t);this.value=this._percentageToValue(i);const r=this._steppedValue(this._boundedValue(this.value));this.dispatchEvent(new CustomEvent("slider-moved",{bubbles:!0,composed:!0,detail:{value:r}}))}),this._mc.on("panend",t=>{if(this.disabled)return;this.pressed=!1,this._hideTooltip();const i=this._getPercentageFromEvent(t),r=this._percentageToValue(i);this.value=this._steppedValue(this._boundedValue(r)),this.dispatchEvent(new CustomEvent("slider-moved",{bubbles:!0,composed:!0,detail:{value:void 0}})),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:this.value}}))}),this._mc.on("singletap",t=>{if(this.disabled)return;const i=this._getPercentageFromEvent(t),r=this._percentageToValue(i);this.value=this._steppedValue(this._boundedValue(r)),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:this.value}}))}),this.addEventListener("keydown",this._handleKeyDown),this.addEventListener("keyup",this._handleKeyUp)}}destroyListeners(){this._mc&&(this._mc.destroy(),this._mc=void 0),this.removeEventListener("keydown",this._handleKeyDown),this.removeEventListener("keyup",this._handleKeyUp)}get _tenPercentStep(){return Math.max(this.step,(this.max-this.min)/10)}_showTooltip(){this._tooltipTimeout!=null&&window.clearTimeout(this._tooltipTimeout),this.tooltipVisible=!0}_hideTooltip(e){if(!e){this.tooltipVisible=!1;return}this._tooltipTimeout=window.setTimeout(()=>{this.tooltipVisible=!1},e)}_renderTooltip(){if(this.tooltipMode==="never")return m;const e=this.vertical?"left":"top",t=this.tooltipMode==="always"||this.tooltipVisible&&this.tooltipMode==="interaction",i=this._boundedValue(this._steppedValue(this.value??0));return h`
      <div
        class="tooltip ${T({visible:t,[e]:!0})}"
      >
        ${i}
      </div>
    `}render(){return h`
      <div
        class="container ${T({pressed:this.pressed})}"
        style=${P({"--value":`${this._valueToPercentage(this.value??0)}`,"--min":`${this.min/this._range}`,"--max":`${1-this.max/this._range}`})}
      >
        <div id="slider" class="slider">
          <div class="track">
            <div class="background"></div>
            <div class="active"></div>
            ${Array(this._range/this.step).fill(0).map((e,t)=>this.min>=t*this.step||t*this.step>this.max?m:h`
                  <div
                    class="dot"
                    style=${P({"--value":`${t/(this._range/this.step)}`})}
                  ></div>
                `)}
            ${this.value!==void 0?h`<div class="handle"></div>`:m}
          </div>
          ${this._renderTooltip()}
        </div>
      </div>
    `}};ge.styles=w`
    :host {
      display: block;
      --grid-layout-slider: 36px;
      height: var(--grid-layout-slider);
      width: 100%;
      outline: none;
      transition: box-shadow 180ms ease-in-out;
    }
    :host(:focus-visible) {
      box-shadow: 0 0 0 2px var(--primary-color);
    }
    :host([vertical]) {
      width: var(--grid-layout-slider);
      height: 100%;
    }
    .container {
      position: relative;
      height: 100%;
      width: 100%;
    }
    .slider {
      position: relative;
      height: 100%;
      width: 100%;
      transform: translateZ(0);
      overflow: visible;
      cursor: pointer;
    }
    .slider * {
      pointer-events: none;
      user-select: none;
    }
    .track {
      position: absolute;
      inset: 0;
      margin: auto;
      height: 16px;
      width: 100%;
      border-radius: var(--ha-border-radius-md, 8px);
      overflow: hidden;
    }
    :host([vertical]) .track {
      width: 16px;
      height: 100%;
    }
    .background {
      position: absolute;
      inset: 0;
      background: var(--disabled-color, #9e9e9e);
      opacity: 0.4;
    }
    .active {
      position: absolute;
      background: var(--primary-color);
      top: 0;
      right: calc(var(--max) * 100%);
      bottom: 0;
      left: calc(var(--min) * 100%);
    }
    :host([vertical]) .active {
      top: calc(var(--min) * 100%);
      right: 0;
      bottom: calc(var(--max) * 100%);
      left: 0;
    }
    .handle {
      position: absolute;
      top: 0;
      height: 100%;
      width: 16px;
      transform: translate(-50%, 0);
      background: var(--card-background-color);
      left: calc(var(--value, 0%) * 100%);
      transition:
        left 180ms ease-in-out,
        top 180ms ease-in-out;
    }
    :host([vertical]) .handle {
      transform: translate(0, -50%);
      left: 0;
      top: calc(var(--value, 0%) * 100%);
      height: 16px;
      width: 100%;
    }
    .dot {
      position: absolute;
      top: 0;
      bottom: 0;
      opacity: 0.6;
      margin: auto;
      width: 4px;
      height: 4px;
      flex-shrink: 0;
      transform: translate(-50%, 0);
      background: var(--card-background-color);
      left: calc(var(--value, 0%) * 100%);
      border-radius: 2px;
    }
    :host([vertical]) .dot {
      transform: translate(0, -50%);
      left: 0;
      right: 0;
      bottom: inherit;
      top: calc(var(--value, 0%) * 100%);
    }
    .handle::after {
      position: absolute;
      inset: 0;
      width: 4px;
      border-radius: 2px;
      height: 100%;
      margin: auto;
      background: var(--primary-color);
      content: "";
    }
    :host([vertical]) .handle::after {
      height: 4px;
      width: 100%;
    }
    :host([disabled]) .slider {
      cursor: not-allowed;
    }
    :host([disabled]) .track {
      opacity: 0.5;
    }
    :host([disabled]) .handle::after {
      background: var(--disabled-color, #9e9e9e);
    }
    :host([disabled]) .active {
      background: var(--disabled-color, #9e9e9e);
    }
    .tooltip {
      position: absolute;
      background-color: var(--card-background-color, #fff);
      color: var(--primary-text-color);
      font-size: 12px;
      border-radius: var(--ha-border-radius-lg, 12px);
      padding: 0.2em 0.4em;
      opacity: 0;
      white-space: nowrap;
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
      transition:
        opacity 180ms ease-in-out,
        left 180ms ease-in-out,
        bottom 180ms ease-in-out;
      --slider-tooltip-position: calc(var(--value) * 100%);
    }
    .tooltip.visible {
      opacity: 1;
    }
    .tooltip.top {
      transform: translate3d(-50%, -100%, 0);
      top: -4px;
      left: var(--slider-tooltip-position);
    }
    .tooltip.left {
      transform: translate3d(-100%, -50%, 0);
      top: var(--slider-tooltip-position);
      left: -4px;
    }
    .pressed .handle {
      transition: none;
    }
    .pressed .tooltip {
      transition: opacity 180ms ease-in-out;
    }
  `;De([u({type:Boolean,reflect:!0})],ge.prototype,"disabled",2);De([u({type:Boolean,reflect:!0})],ge.prototype,"vertical",2);De([u({attribute:"touch-action"})],ge.prototype,"touchAction",2);De([u({attribute:"tooltip-mode"})],ge.prototype,"tooltipMode",2);De([u({type:Number})],ge.prototype,"value",2);De([u({type:Number})],ge.prototype,"step",2);De([u({type:Number})],ge.prototype,"min",2);De([u({type:Number})],ge.prototype,"max",2);De([u({type:Number})],ge.prototype,"range",2);De([g()],ge.prototype,"pressed",2);De([g()],ge.prototype,"tooltipVisible",2);De([mi("#slider")],ge.prototype,"slider",2);ge=De([x("ha-grid-layout-slider")],ge);var Fy=Object.defineProperty,By=Object.getOwnPropertyDescriptor,oa=(e,t,i,r)=>{for(var o=r>1?void 0:r?By(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Fy(t,i,o),o};let ro=class extends y{constructor(){super(...arguments),this.path="",this.label="",this.disabled=!1}render(){const e=this.path||Ri("mdi:help-circle-outline");return h`
      <button
        type="button"
        class="button"
        ?disabled=${this.disabled}
        aria-label=${this.label||m}
        title=${this.label||m}
      >
        <ha-svg-icon .path=${e}></ha-svg-icon>
      </button>
    `}};ro.styles=w`
    :host {
      display: inline-flex;
    }
    .button {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 40px;
      height: 40px;
      border: none;
      border-radius: 50%;
      background: transparent;
      color: inherit;
      cursor: pointer;
      padding: 0;
    }
    .button:hover:not(:disabled) {
      background: color-mix(in srgb, currentColor 8%, transparent);
    }
    .button:disabled {
      opacity: 0.38;
      cursor: default;
    }
    ha-svg-icon {
      width: 24px;
      height: 24px;
    }
  `;oa([u()],ro.prototype,"path",2);oa([u()],ro.prototype,"label",2);oa([u({type:Boolean})],ro.prototype,"disabled",2);ro=oa([x("ha-icon-button")],ro);var Uy=Object.defineProperty,jy=Object.getOwnPropertyDescriptor,lt=(e,t,i,r)=>{for(var o=r>1?void 0:r?jy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Uy(t,i,o),o};function sh(e,t,i){let r=e;return t!==void 0&&(r=Math.max(r,t)),i!==void 0&&(r=Math.min(r,i)),r}let Oe=class extends y{constructor(){super(...arguments),this.rows=8,this.columns=12,this.isDefault=!1,this.step=1,this._localValue={rows:1,columns:1}}willUpdate(e){e.has("value")&&(this._localValue=this.value)}_cellClick(e){const t=e.currentTarget,i=Number(t.getAttribute("data-row")),r=Number(t.getAttribute("data-column")),o=sh(i,this.rowMin,this.rowMax);let n=sh(r,this.columnMin,this.columnMax);(this.value??Or).columns==="full"&&n===this.columns&&(n="full"),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:{rows:o,columns:n}}}))}_valueChanged(e){e.stopPropagation();const t=e.currentTarget.id,i=this.value??Or;let r=e.detail.value;t==="columns"&&i.columns==="full"&&r===this.columns&&(r="full"),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:{...i,[t]:r}}}))}_reset(e){e.stopPropagation(),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:{rows:void 0,columns:void 0}}}))}_sliderMoved(e){e.stopPropagation();const t=e.currentTarget.id,i=this.value??Or,r=e.detail.value;r!==void 0&&(this._localValue={...i,[t]:r})}render(){var f,b,v,_,k,C;const e=((f=this._localValue)==null?void 0:f.rows)==="auto",t=((b=this._localValue)==null?void 0:b.columns)==="full",i=t||this.columnMin!==void 0&&this.columnMin===this.columnMax,r=e||this.rowMin!==void 0&&this.rowMin===this.rowMax,o=this.rowMin??1,n=this.rowMax??this.rows,s=Math.ceil((this.columnMin??1)/this.step)*this.step,a=Math.ceil((this.columnMax??this.columns)/this.step)*this.step,l=e?o:(v=this._localValue)==null?void 0:v.rows,c=(_=this._localValue)==null?void 0:_.columns,d=e?1:Number((k=this._localValue)==null?void 0:k.rows)||1,p=t?this.columns:Number((C=this._localValue)==null?void 0:C.columns)||1;return h`
      <div
        class="grid"
        style=${P({"--preview-rows":String(this.rows)})}
      >
        <ha-grid-layout-slider
          id="columns"
          .value=${typeof c=="number"?c:this.columns}
          .min=${s}
          .max=${a}
          .range=${this.columns}
          .step=${this.step}
          .disabled=${i}
          @slider-moved=${this._sliderMoved}
          @value-changed=${this._valueChanged}
        ></ha-grid-layout-slider>

        <div class="reset">
          ${this.isDefault?m:h`
                <ha-icon-button
                  .path=${Ef}
                  label="恢复默认"
                  @click=${this._reset}
                ></ha-icon-button>
              `}
        </div>

        <div
          class="preview"
          style=${P({"--rows":String(d),"--columns":String(p),"--total-columns":String(this.columns)})}
        >
          <table>
            ${Array(this.rows).fill(0).map((O,I)=>{const V=I+1;return h`
                  <tr>
                    ${Array(this.columns).fill(0).map((K,R)=>{const Q=R+1;return Q%this.step!==0||this.columns>24&&Q%3!==0?m:h`
                          <td
                            data-row=${V}
                            data-column=${Q}
                            @click=${this._cellClick}
                          ></td>
                        `})}
                  </tr>
                `})}
          </table>
          <div class="preview-card"></div>
        </div>

        <ha-grid-layout-slider
          vertical
          id="rows"
          .value=${typeof l=="number"?l:o}
          .min=${o}
          .max=${n}
          .range=${this.rows}
          .disabled=${r}
          @slider-moved=${this._sliderMoved}
          @value-changed=${this._valueChanged}
        ></ha-grid-layout-slider>
      </div>
    `}};Oe.styles=w`
    .grid {
      display: grid;
      grid-template-areas:
        "reset column-slider"
        "row-slider preview";
      grid-template-rows: auto auto;
      grid-template-columns: auto 1fr;
      gap: var(--ha-space-2, 8px);
      direction: ltr;
    }
    #columns {
      grid-area: column-slider;
    }
    #rows {
      grid-area: row-slider;
      height: calc(var(--preview-rows, 8) * 30px);
      align-self: stretch;
    }
    .reset {
      grid-area: reset;
      --ha-icon-button-size: 36px;
    }
    .preview {
      position: relative;
      grid-area: preview;
    }
    .preview table,
    .preview tr,
    .preview td {
      border: 2px dotted var(--divider-color);
      border-collapse: collapse;
    }
    .preview table {
      width: 100%;
    }
    .preview tr {
      height: 30px;
    }
    .preview td {
      cursor: pointer;
    }
    .preview-card {
      position: absolute;
      top: 0;
      left: 0;
      background-color: var(--primary-color);
      opacity: 0.3;
      border-radius: var(--ha-border-radius-md, 8px);
      height: calc(var(--rows, 1) * 30px);
      width: calc(var(--columns, 1) * 100% / var(--total-columns, 12));
      pointer-events: none;
      transition:
        width ease-in-out 180ms,
        height ease-in-out 180ms;
    }
  `;lt([u({attribute:!1})],Oe.prototype,"value",2);lt([u({type:Number})],Oe.prototype,"rows",2);lt([u({type:Number})],Oe.prototype,"columns",2);lt([u({type:Number})],Oe.prototype,"rowMin",2);lt([u({type:Number})],Oe.prototype,"rowMax",2);lt([u({type:Number})],Oe.prototype,"columnMin",2);lt([u({type:Number})],Oe.prototype,"columnMax",2);lt([u({type:Boolean})],Oe.prototype,"isDefault",2);lt([u({type:Number})],Oe.prototype,"step",2);lt([g()],Oe.prototype,"_localValue",2);Oe=lt([x("ha-grid-size-picker")],Oe);var qy=Object.defineProperty,Gy=Object.getOwnPropertyDescriptor,Pn=(e,t,i,r)=>{for(var o=r>1?void 0:r?Gy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&qy(t,i,o),o};let sr=class extends y{constructor(){super(...arguments),this._preciseMode=!1}async _loadDefaults(){var e;try{const t=await Ks(this.config);this._defaultGridOptions=((e=t.getGridOptions)==null?void 0:e.call(t))??{}}catch{this._defaultGridOptions={}}}firstUpdated(){this._loadDefaults()}willUpdate(e){if(e.has("config")){const t=this.config.grid_options;if(!t){this._preciseMode=this._defaultGridOptions?Hd(this._defaultGridOptions):!1;return}const i=Hd(t);!this._preciseMode&&i&&(this._preciseMode=i)}}updated(e){e.has("config")&&this._loadDefaults()}_configGridOptions(e){return e.grid_options?e.grid_options:e.layout_options?Xp(e.layout_options):{}}_mergedOptions(e,t){return{...Or,...t,...e}}_isDefault(e){return(e==null?void 0:e.columns)===void 0&&(e==null?void 0:e.rows)===void 0}_updateGridOptions(e){const t={...this.config,grid_options:{...e}};if(t.grid_options){for(const[i,r]of Object.entries(t.grid_options))r===void 0&&delete t.grid_options[i];Object.keys(t.grid_options).length===0&&delete t.grid_options}t.layout_options&&delete t.layout_options,this.dispatchEvent(new CustomEvent("config-changed",{bubbles:!0,composed:!0,detail:{config:t}}))}_gridSizeChanged(e){e.stopPropagation();const t=e.detail.value;this._updateGridOptions({...this.config.grid_options,columns:t.columns,rows:t.rows})}_fullWidthChanged(e){var o;e.stopPropagation();const t=e.target.checked,i={...Or,...this._defaultGridOptions};let r;if(t)r="full";else if(i.columns==="full"){const s=12*(((o=this.sectionConfig)==null?void 0:o.column_span)??1);r=i.max_columns??s}else r=void 0;this._updateGridOptions({...this.config.grid_options,columns:r})}_autoHeightChanged(e){e.stopPropagation();const t=e.target.checked,i={...Or,...this._defaultGridOptions};let r;t?r="auto":i.rows==="auto"?r=i.min_rows??1:r=void 0,this._updateGridOptions({...this.config.grid_options,rows:r})}_preciseModeChanged(e){var i;if(e.stopPropagation(),this._preciseMode=e.target.checked,this._preciseMode)return;const t=(i=this.config.grid_options)==null?void 0:i.columns;if(typeof t=="number"&&t%ai!==0){const r=Math.ceil(t/ai)*ai;this._updateGridOptions({...this.config.grid_options,columns:r})}}render(){var l;const e=this._configGridOptions(this.config),t=this._mergedOptions(e,this._defaultGridOptions),i=Wp(t),o=12*(((l=this.sectionConfig)==null?void 0:l.column_span)??1),n=t.rows==="auto",s=t.columns==="full",a=this._defaultGridOptions!==void 0&&Object.keys(this._defaultGridOptions).length===0;return h`
      ${a?h`
            <div class="alert">
              此卡片未声明网格尺寸默认值，布局调整可能不完全生效。
            </div>
          `:m}

      <ha-grid-size-picker
        class=${a?"disabled":""}
        .value=${i}
        .rows=${8}
        .columns=${o}
        .rowMin=${t.min_rows}
        .rowMax=${t.max_rows??8}
        .columnMin=${t.min_columns}
        .columnMax=${t.max_columns??o}
        .step=${this._preciseMode?1:ai}
        .isDefault=${this._isDefault(e)}
        @value-changed=${this._gridSizeChanged}
      ></ha-grid-size-picker>

      <label class="switch-row">
        <div>
          <div class="switch-label">自动高度</div>
          <div class="switch-hint">根据卡片内容调整卡片高度</div>
        </div>
        <input
          type="checkbox"
          role="switch"
          .checked=${n}
          @change=${this._autoHeightChanged}
        />
      </label>

      <label class="switch-row">
        <div>
          <div class="switch-label">全宽</div>
          <div class="switch-hint">占据整个部件的宽度，无论其大小</div>
        </div>
        <input
          type="checkbox"
          role="switch"
          .checked=${s}
          @change=${this._fullWidthChanged}
        />
      </label>

      <label class="switch-row">
        <div>
          <div class="switch-label">精确模式</div>
          <div class="switch-hint">更精确地更改卡片宽度（步进 1 而非 3）</div>
        </div>
        <input
          type="checkbox"
          role="switch"
          .checked=${this._preciseMode}
          @change=${this._preciseModeChanged}
        />
      </label>
    `}};sr.styles=w`
    :host {
      display: block;
    }
    .alert {
      padding: 12px;
      margin-bottom: 12px;
      border-radius: 8px;
      background: color-mix(in srgb, var(--primary-color) 12%, transparent);
      color: var(--primary-text-color);
      font-size: 13px;
    }
    ha-grid-size-picker {
      display: block;
      margin: 16px auto;
      direction: ltr;
    }
    ha-grid-size-picker.disabled {
      opacity: 0.5;
      pointer-events: none;
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      cursor: pointer;
      padding: 12px 0;
      border-top: 1px solid var(--divider-color);
    }
    .switch-label {
      font-size: 14px;
      font-weight: 500;
      color: var(--primary-text-color);
    }
    .switch-hint {
      font-size: 12px;
      color: var(--secondary-text-color);
      margin-top: 2px;
      line-height: 1.35;
    }
    .switch-row input[type="checkbox"] {
      appearance: none;
      width: 36px;
      height: 20px;
      border-radius: 999px;
      background: var(--disabled-color, #9e9e9e);
      position: relative;
      flex-shrink: 0;
      cursor: pointer;
      transition: background 180ms ease;
    }
    .switch-row input[type="checkbox"]::after {
      content: "";
      position: absolute;
      top: 2px;
      left: 2px;
      width: 16px;
      height: 16px;
      border-radius: 50%;
      background: #fff;
      transition: transform 180ms ease;
    }
    .switch-row input[type="checkbox"]:checked {
      background: var(--primary-color);
    }
    .switch-row input[type="checkbox"]:checked::after {
      transform: translateX(16px);
    }
  `;Pn([u({attribute:!1})],sr.prototype,"config",2);Pn([u({attribute:!1})],sr.prototype,"sectionConfig",2);Pn([g()],sr.prototype,"_defaultGridOptions",2);Pn([g()],sr.prototype,"_preciseMode",2);sr=Pn([x("flow-card-layout-editor")],sr);var Xy=Object.defineProperty,Wy=Object.getOwnPropertyDescriptor,vr=(e,t,i,r)=>{for(var o=r>1?void 0:r?Wy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Xy(t,i,o),o};let jt=class extends y{constructor(){super(...arguments),this.open=!1,this._draft={type:"tile"},this._tab="config",this._layoutRevision=0,this._backdropDismiss=mr(()=>this._close())}updated(e){e.has("open")&&this.open&&(this._tab="config"),e.has("config")&&this.config&&(this._draft=structuredClone(this.config),this._layoutRevision+=1)}_close(){this.dispatchEvent(new CustomEvent("close",{bubbles:!0,composed:!0}))}_save(){this.dispatchEvent(new CustomEvent("save",{bubbles:!0,composed:!0,detail:{config:structuredClone(this._draft)}}))}_onConfigChanged(e){e.stopPropagation(),this._draft=e.detail.config,this._layoutRevision+=1}_onPicturePosition(e){if(e.stopPropagation(),this._tab!=="config")return;const t=this.renderRoot.querySelector("flow-card-config-editor");t==null||t.applyPictureElementPosition(e.detail.left,e.detail.top)}_onPictureElementSelect(e){if(e.stopPropagation(),this._tab!=="config")return;const t=this.renderRoot.querySelector("flow-card-config-editor");t==null||t.selectPictureElement(e.detail.index)}_onPictureElementsChanged(e){e.stopPropagation(),this._draft=e.detail.config,this._layoutRevision+=1}_previewSectionConfig(){const e=this.sectionConfig;if(e){const{cards:t,title:i,...r}=e;return{...r,type:e.type??"grid",column_span:1,cards:[this._draft]}}return{type:"grid",column_span:1,cards:[this._draft]}}_cardTypeName(){return{tile:"Toggle",heading:"Title",sensor:"数值",switch:"开关",trigger:"触发器",climate:"步进",cover:"单选",clock:"Clock",slider:"滑块",gain:"增益",media:"媒体",markdown:"Markdown",link:"超链接","picture-elements":"图片元素","path-badge":"轨迹元素",line2d:"2D 折线",scatter2d:"2D 散点",bar:"柱状图",line3d:"3D 折线",scatter3d:"3D 散点"}[String(this._draft.type)]??String(this._draft.type)}render(){if(!this.open)return m;const e=this._previewSectionConfig();return h`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog">
          <header class="header">
            <h3>${this._cardTypeName()} 卡片配置</h3>
            <button type="button" class="close" @click=${this._close} aria-label="关闭">×</button>
          </header>

          <nav class="tabs">
            <button
              type="button"
              class="tab ${this._tab==="config"?"active":""}"
              @click=${()=>{this._tab="config"}}
            >
              配置
            </button>
            <button
              type="button"
              class="tab ${this._tab==="layout"?"active":""}"
              @click=${()=>{this._tab="layout"}}
            >
              布局
            </button>
          </nav>

          <div class="body">
            <div class="editor-pane">
              ${this._tab==="config"?h`
                    <flow-card-config-editor
                      .config=${this._draft}
                      @config-changed=${this._onConfigChanged}
                    ></flow-card-config-editor>
                  `:h`
                    <flow-card-layout-editor
                      .config=${this._draft}
                      .sectionConfig=${this.sectionConfig}
                      @config-changed=${this._onConfigChanged}
                    ></flow-card-layout-editor>
                  `}
            </div>

            <div
              class="preview-pane element-preview"
              @picture-element-position=${this._onPicturePosition}
              @picture-element-select=${this._onPictureElementSelect}
              @picture-elements-changed=${this._onPictureElementsChanged}
            >
              <div class="preview-label">预览</div>
              <flow-section
                class="preview-section"
                style="--column-span: 1;"
                .config=${e}
                .editMode=${!1}
                .preview=${!0}
                .layoutRevision=${this._layoutRevision}
              ></flow-section>
            </div>
          </div>

          <div class="actions">
            <button type="button" class="secondary" @click=${this._close}>取消</button>
            <button type="button" class="primary" @click=${this._save}>保存</button>
          </div>
        </div>
      </div>
    `}};jt.styles=w`
    .backdrop {
      position: fixed;
      inset: 0;
      background: rgba(15, 23, 42, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 200;
      padding: 16px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(920px, 100%);
      max-height: min(90vh, 720px);
      background: var(--card-background-color);
      border-radius: 16px;
      display: flex;
      flex-direction: column;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.18);
      overflow: hidden;
    }
    .header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 16px 20px 0;
    }
    h3 {
      margin: 0;
      font-size: 18px;
      font-weight: 600;
    }
    .close {
      border: none;
      background: transparent;
      font-size: 24px;
      line-height: 1;
      cursor: pointer;
      color: var(--secondary-text-color);
      padding: 4px 8px;
    }
    .tabs {
      display: flex;
      gap: 0;
      padding: 0 20px;
      border-bottom: 1px solid var(--divider-color);
      margin-top: 12px;
    }
    .tab {
      border: none;
      background: transparent;
      padding: 12px 16px;
      font-size: 14px;
      font-weight: 500;
      color: var(--secondary-text-color);
      cursor: pointer;
      border-bottom: 2px solid transparent;
      margin-bottom: -1px;
    }
    .tab.active {
      color: var(--primary-color);
      border-bottom-color: var(--primary-color);
    }
    .body {
      display: flex;
      flex: 1;
      min-height: 0;
      overflow: hidden;
    }
    .editor-pane {
      flex: 1;
      padding: 20px;
      overflow: auto;
      min-width: 0;
    }
    /* HA .element-preview */
    .preview-pane {
      flex: 0 0 min(320px, 40%);
      border-left: 1px solid var(--divider-color);
      background: var(--primary-background-color);
      padding: 12px;
      display: flex;
      flex-direction: column;
      gap: 8px;
      overflow: auto;
      min-width: 0;
      box-sizing: border-box;
    }
    .preview-label {
      font-size: 12px;
      font-weight: 600;
      color: var(--secondary-text-color);
      text-transform: uppercase;
      letter-spacing: 0.04em;
      flex: none;
    }
    /*
      HA: .content hui-section {
        max-width: var(--ha-view-sections-column-max-width, 500px);
        margin: 0 auto;
      }
      Card keeps its grid_options size inside the section — no stretch fill.
    */
    .preview-section {
      display: block;
      width: 100%;
      max-width: var(--ha-view-sections-column-max-width, 500px);
      margin: 0 auto;
      padding: 4px;
      box-sizing: border-box;
    }
    .actions {
      display: flex;
      justify-content: flex-end;
      gap: 8px;
      padding: 12px 20px 16px;
      border-top: 1px solid var(--divider-color);
    }
    button {
      border-radius: 8px;
      padding: 8px 14px;
      font-weight: 600;
      cursor: pointer;
      border: 1px solid var(--divider-color);
      background: #fff;
    }
    button.primary {
      background: var(--primary-color);
      border-color: var(--primary-color);
      color: #fff;
    }
    @media (min-width: 1000px) {
      .dialog {
        width: min(1100px, 100%);
      }
      .preview-pane {
        flex: 1 1 0;
        max-width: 520px;
      }
      .preview-section {
        padding: 8px 10px;
      }
    }
    @media (max-width: 720px) {
      .body {
        flex-direction: column;
      }
      .preview-pane {
        flex: none;
        max-width: none;
        border-left: none;
        border-top: 1px solid var(--divider-color);
      }
    }
  `;vr([cr({context:vi,subscribe:!0}),u({attribute:!1})],jt.prototype,"flow",2);vr([u({type:Boolean})],jt.prototype,"open",2);vr([u({attribute:!1})],jt.prototype,"config",2);vr([u({attribute:!1})],jt.prototype,"sectionConfig",2);vr([g()],jt.prototype,"_draft",2);vr([g()],jt.prototype,"_tab",2);jt=vr([x("flow-card-editor")],jt);class Yy{constructor(t=2){this.maxConcurrent=t,this.active=0,this.pending=[]}enqueue(t){this.pending.push(t),this.pump()}pump(){for(;this.active<this.maxConcurrent&&this.pending.length>0;){const t=this.pending.shift();if(!t)return;this.active+=1,t().finally(()=>{this.active-=1,this.pump()})}}}const Zy=new Yy(3);var Ky=Object.defineProperty,Qy=Object.getOwnPropertyDescriptor,ve=(e,t,i,r)=>{for(var o=r>1?void 0:r?Qy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ky(t,i,o),o};const Xn=6;function Jy(e,t){const i=t.trim().toLowerCase();return i?e.filter(r=>[r.type,r.name,r.description??""].join(" ").toLowerCase().includes(i)):e}let rt=class extends y{constructor(){super(...arguments),this.type="",this.name="",this.icon="mdi:card-outline",this.index=0,this._previewState="placeholder",this._loadRequested=!1,this._disconnected=!1}connectedCallback(){super.connectedCallback(),this._disconnected=!1,this.index<Xn&&this._scheduleVisibilityCheck(),this._bindScrollRoot()}disconnectedCallback(){this._disconnected=!0,this._teardownObserver(),super.disconnectedCallback()}updated(e){e.has("scrollRoot")&&this._bindScrollRoot(),(e.has("config")||e.has("type"))&&(this._loadRequested=!1,this._card=void 0,this._previewState="placeholder",this._scheduleVisibilityCheck()),e.has("index")&&this.index<Xn&&this._scheduleVisibilityCheck(),this._previewState==="ready"&&this._card&&requestAnimationFrame(()=>this._mountPreview())}_teardownObserver(){var e;(e=this._observer)==null||e.disconnect(),this._observer=void 0,this._scrollListener&&this.scrollRoot&&this.scrollRoot.removeEventListener("scroll",this._scrollListener),this._scrollListener=void 0}_bindScrollRoot(){this._teardownObserver(),this.scrollRoot&&(this._observer=new IntersectionObserver(e=>{e.some(t=>t.isIntersecting)&&this._requestPreview()},{root:this.scrollRoot,rootMargin:"100px 0px",threshold:.01}),this._observer.observe(this),this._scrollListener=()=>this._scheduleVisibilityCheck(),this.scrollRoot.addEventListener("scroll",this._scrollListener,{passive:!0}),this._scheduleVisibilityCheck())}_scheduleVisibilityCheck(){requestAnimationFrame(()=>{this._disconnected||this._loadRequested||(this.index<Xn||this._isVisibleInScrollRoot())&&this._requestPreview()})}_isVisibleInScrollRoot(){if(!this.scrollRoot)return this.index<Xn;const e=this.getBoundingClientRect();if(e.width<=0||e.height<=0)return!1;const t=this.scrollRoot.getBoundingClientRect();return e.bottom>t.top&&e.top<t.bottom}_requestPreview(){this._loadRequested||(this._loadRequested=!0,this._teardownObserver(),this._previewState!=="ready"&&(this._previewState="loading",Zy.enqueue(()=>this._loadPreview())))}async _loadPreview(){var e,t;try{const i=((e=this.config)==null?void 0:e.type)||this.type,r=await ml(i);if(this._disconnected)return;if(!r){this._previewState="error";return}const o=this.config??((t=r.getStubConfig)==null?void 0:t.call(r))??{type:i},n=await Ks(o);if(this._disconnected)return;n.flow=this.flow,this._card=n,this._previewState="ready"}catch{this._disconnected||(this._previewState="error")}}_mountPreview(){if(!this._card||this._previewState!=="ready")return;this._card.flow=this.flow;const e=this.renderRoot.querySelector(".preview-inner");e&&!e.contains(this._card)&&e.replaceChildren(this._card)}_pick(){var e;this.dispatchEvent(new CustomEvent("card-select",{detail:{type:((e=this.config)==null?void 0:e.type)||this.type,config:this.config?structuredClone(this.config):void 0},bubbles:!0,composed:!0}))}render(){return h`
      <button type="button" class="card" @click=${this._pick}>
        <div class="card-header">${this.name}</div>
        <div class="preview">
          ${this._previewState==="ready"?h`<div class="preview-inner"></div>`:this._previewState==="loading"?h`<div class="loading" aria-label="加载预览"></div>`:h`
                  <div class="preview-icon">
                    <ha-icon .icon=${this.icon}></ha-icon>
                  </div>
                `}
        </div>
      </button>
    `}};rt.styles=w`
    :host {
      display: block;
      min-width: 0;
    }
    .card {
      width: 100%;
      border: 1px solid var(--divider-color, #e0e0e0);
      border-radius: 12px;
      overflow: hidden;
      cursor: pointer;
      background: var(--card-background-color, #fff);
      padding: 0;
      text-align: inherit;
      font: inherit;
      color: inherit;
      transition:
        border-color 120ms ease,
        box-shadow 120ms ease;
    }
    .card:hover,
    .card:focus-visible {
      border-color: var(--primary-color, #03a9f4);
      box-shadow: 0 0 0 1px var(--primary-color, #03a9f4);
      outline: none;
    }
    .card-header {
      text-align: start;
      padding: 10px 12px 8px;
      font-size: 13px;
      font-weight: 500;
      color: var(--secondary-text-color, #64748b);
      background: var(--card-background-color, #fff);
    }
    .preview {
      height: 200px;
      overflow: hidden;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 0 10px 10px;
      box-sizing: border-box;
      background: var(--primary-background-color, #fafafa);
    }
    .preview-inner {
      width: 100%;
      max-width: 280px;
      transform: scale(0.95);
      transform-origin: center center;
      pointer-events: none;
    }
    .preview-inner > * {
      width: 100%;
    }
    .preview-icon {
      width: 56px;
      height: 56px;
      border-radius: 16px;
      display: flex;
      align-items: center;
      justify-content: center;
      background: rgba(3, 169, 244, 0.08);
      color: var(--primary-color, #03a9f4);
      --mdc-icon-size: 30px;
    }
    .loading {
      width: 28px;
      height: 28px;
      border-radius: 50%;
      border: 2px solid var(--divider-color, #e0e0e0);
      border-top-color: var(--primary-color, #03a9f4);
      animation: spin 0.8s linear infinite;
    }
    @keyframes spin {
      to {
        transform: rotate(360deg);
      }
    }
  `;ve([u()],rt.prototype,"type",2);ve([u()],rt.prototype,"name",2);ve([u()],rt.prototype,"icon",2);ve([u({attribute:!1})],rt.prototype,"config",2);ve([u({type:Number})],rt.prototype,"index",2);ve([u({attribute:!1})],rt.prototype,"flow",2);ve([u({attribute:!1})],rt.prototype,"scrollRoot",2);ve([g()],rt.prototype,"_previewState",2);rt=ve([x("flow-card-picker-item")],rt);let fi=class extends y{constructor(){super(...arguments),this.open=!1,this._query="",this._backdropDismiss=mr(()=>this._close())}connectedCallback(){super.connectedCallback(),this._refreshClipboard(),this._unsubClipboard=Rv(()=>this._refreshClipboard())}disconnectedCallback(){var e;(e=this._unsubClipboard)==null||e.call(this),super.disconnectedCallback()}firstUpdated(){this._syncScrollRoot()}updated(e){e.has("open")&&this.open&&(this._query="",this._refreshClipboard(),this._syncScrollRoot())}_refreshClipboard(){this._clipboard=Vv()}_syncScrollRoot(){const e=this.renderRoot.querySelector(".body");e&&e!==this._scrollRoot&&(this._scrollRoot=e)}_close(){this.dispatchEvent(new CustomEvent("close",{bubbles:!0,composed:!0}))}_onCardSelect(e){e.stopPropagation(),this.dispatchEvent(new CustomEvent("card-picked",{detail:e.detail,bubbles:!0,composed:!0}))}_onSearchInput(e){this._query=e.target.value}_allCards(){return gh()}_filteredCards(){return Jy(this._allCards(),this._query)}_suggestedEntries(e=xd){const t=this._allCards(),i=new Map(t.map(a=>[a.type,a])),r=Ap().map(a=>i.get(a)).filter(a=>!!a),o=Bv.map(a=>i.get(a)).filter(a=>!!a),n=new Set,s=[];for(const a of[...r,...o])if(!n.has(a.type)&&(n.add(a.type),s.push(a),s.length>=e))break;return s}_renderItem(e,t,i){return h`
      <flow-card-picker-item
        .type=${e.type}
        .name=${(i==null?void 0:i.name)??e.name}
        .icon=${e.icon??"mdi:card-outline"}
        .config=${i==null?void 0:i.config}
        .index=${t}
        .flow=${this.flow}
        .scrollRoot=${this._scrollRoot}
        @card-select=${this._onCardSelect}
      ></flow-card-picker-item>
    `}_renderGrid(e,t=0,i){return e.length?h`
      <div class="cards">
        ${e.map((r,o)=>{var n,s;return this._renderItem(r,t+o,{config:(n=i==null?void 0:i.configForType)==null?void 0:n.call(i,r.type),name:(s=i==null?void 0:i.nameForType)==null?void 0:s.call(i,r.type)})})}
      </div>
    `:h`<div class="empty">没有匹配的卡片</div>`}_renderClipboardCard(e){const t=this._clipboard;if(!t)return m;const i=fh(t.type)??{type:t.type,name:t.type,icon:"mdi:content-paste",category:"container"};return this._renderItem(i,e,{config:t,name:"从剪贴板粘贴"})}_renderSections(){if(this._query.trim())return this._renderGrid(this._filteredCards());const t=!!this._clipboard,i=this._suggestedEntries(Math.max(0,xd-(t?1:0))),r=this._allCards();let o=0;return h`
      ${t||i.length?h`
            <ha-expandable-panel .header=${"推荐卡片"} expanded>
              <div class="cards">
                ${t?this._renderClipboardCard(o++):m}
                ${i.map(n=>this._renderItem(n,o++))}
              </div>
            </ha-expandable-panel>
          `:m}

      <ha-expandable-panel .header=${"核心卡"} expanded>
        ${!t&&!i.length?h`<div class="cards">${this._renderClipboardCard(0)}</div>`:m}
        ${this._renderGrid(r,o)}
      </ha-expandable-panel>
    `}render(){return this.open?h`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog">
          <header class="header">
            <ha-icon-button
              class="close"
              .path=${mf}
              label="关闭"
              @click=${this._close}
            ></ha-icon-button>
            <h2>添加到仪表板</h2>
          </header>

          <div class="search-wrap">
            <label class="search">
              <ha-svg-icon class="search-icon" .path=${wf}></ha-svg-icon>
              <input
                type="search"
                placeholder="搜索卡片"
                .value=${this._query}
                @input=${this._onSearchInput}
              />
            </label>
          </div>

          <div class="body">
            ${this._renderSections()}
          </div>

          <footer class="actions">
            <button type="button" class="secondary" @click=${this._close}>取消</button>
          </footer>
        </div>
      </div>
    `:m}};fi.styles=w`
    .backdrop {
      position: fixed;
      inset: 0;
      background: rgba(15, 23, 42, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 150;
      padding: 16px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(880px, 100%);
      max-height: min(90vh, 820px);
      background: var(--card-background-color, #fff);
      border-radius: 16px;
      display: flex;
      flex-direction: column;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.18);
      overflow: hidden;
      color: var(--primary-text-color, #1e293b);
      font-family: var(--ha-font-family, "Segoe UI", system-ui, sans-serif);
    }
    .header {
      display: flex;
      align-items: center;
      flex-wrap: wrap;
      gap: 4px;
      padding: 8px 8px 0 4px;
      flex: none;
    }
    h2 {
      margin: 0;
      font-size: 18px;
      font-weight: 600;
      text-align: start;
      flex: 1;
      min-width: 0;
    }
    .close {
      flex: none;
    }
    .search-wrap {
      padding: 16px 20px 0;
      flex: none;
    }
    .search {
      display: flex;
      align-items: center;
      gap: 8px;
      border: 1px solid var(--divider-color, #e0e0e0);
      border-radius: 10px;
      padding: 0 12px;
      background: var(--card-background-color, #fff);
      min-height: 44px;
      box-sizing: border-box;
    }
    .search-icon {
      width: 20px;
      height: 20px;
      color: var(--secondary-text-color, #64748b);
      flex: none;
    }
    .search input {
      border: none;
      background: transparent;
      flex: 1;
      min-width: 0;
      font: inherit;
      font-size: 14px;
      color: inherit;
      outline: none;
    }
    .search input::placeholder {
      color: var(--secondary-text-color, #94a3b8);
    }
    .body {
      flex: 1;
      min-height: 0;
      overflow: auto;
      padding: 8px 20px 8px;
    }
    ha-expandable-panel {
      display: block;
      margin-bottom: 8px;
    }
    .cards {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 16px;
      padding-bottom: 8px;
    }
    @media (max-width: 520px) {
      .cards {
        grid-template-columns: minmax(0, 1fr);
      }
    }
    .empty {
      padding: 32px 0;
      text-align: center;
      color: var(--secondary-text-color, #64748b);
      font-size: 14px;
    }
    .actions {
      display: flex;
      justify-content: flex-end;
      gap: 8px;
      padding: 12px 20px 16px;
      border-top: 1px solid var(--divider-color, #e0e0e0);
      flex: none;
    }
    .secondary {
      border: none;
      background: transparent;
      color: var(--primary-color, #03a9f4);
      border-radius: 999px;
      height: 36px;
      padding: 0 18px;
      cursor: pointer;
      font-size: 14px;
      font-weight: 600;
    }
    .secondary:hover {
      background: rgba(3, 169, 244, 0.08);
    }
  `;ve([u({type:Boolean,reflect:!0})],fi.prototype,"open",2);ve([u({attribute:!1})],fi.prototype,"flow",2);ve([g()],fi.prototype,"_query",2);ve([g()],fi.prototype,"_scrollRoot",2);ve([g()],fi.prototype,"_clipboard",2);fi=ve([x("flow-card-picker")],fi);var ex=Object.defineProperty,tx=Object.getOwnPropertyDescriptor,Ci=(e,t,i,r)=>{for(var o=r>1?void 0:r?tx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ex(t,i,o),o};let wt=class extends y{constructor(){super(...arguments),this.open=!1,this.maxColumns=4,this._columnSpan=1,this._backgroundEnabled=!1,this._backgroundColor="default",this._backgroundOpacity=Zn,this._backdropDismiss=mr(()=>this._close())}updated(e){if(e.has("config")&&this.config){this._columnSpan=this.config.column_span??1;const t=this.config.background;this._backgroundEnabled=t!==void 0,typeof t=="object"&&t?(this._backgroundColor=t.color??"default",this._backgroundOpacity=t.opacity??Zn):(this._backgroundColor="default",this._backgroundOpacity=Zn)}}_pickerColor(){return this._backgroundColor==="default"?"":this._backgroundColor}_close(){this.dispatchEvent(new CustomEvent("close",{bubbles:!0,composed:!0}))}_save(){const e={column_span:this._columnSpan};if(this._backgroundEnabled){const t={opacity:this._backgroundOpacity};this._backgroundColor!=="default"&&(t.color=this._backgroundColor),e.background=t}else e.background=void 0;this.dispatchEvent(new CustomEvent("save",{bubbles:!0,composed:!0,detail:{patch:e}}))}render(){return this.open?h`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog">
          <h3>分区设置</h3>
          <p class="hint">标题请在分区内的「Title」卡片中编辑，与 Home Assistant 一致。</p>

          <label>
            <span>宽度（列数）: ${this._columnSpan}</span>
            <input
              type="range"
              min="1"
              max=${this.maxColumns}
              step="1"
              .value=${String(this._columnSpan)}
              @input=${e=>{this._columnSpan=Number(e.target.value)}}
            />
          </label>

          <label class="checkbox">
            <input
              type="checkbox"
              .checked=${this._backgroundEnabled}
              @change=${e=>{this._backgroundEnabled=e.target.checked}}
            />
            <span>启用背景色</span>
          </label>

          ${this._backgroundEnabled?h`
                <ha-color-picker
                  label="背景颜色"
                  .value=${this._pickerColor()}
                  ?includeState=${!1}
                  ?includeDefault=${!0}
                  defaultLabel="默认"
                  ?hideHelper=${!0}
                  @color-changed=${e=>{const t=e.detail.value;this._backgroundColor=t||"default"}}
                ></ha-color-picker>
                <label>
                  <span>背景不透明度: ${this._backgroundOpacity}%</span>
                  <input
                    type="range"
                    min="0"
                    max="100"
                    step="1"
                    .value=${String(this._backgroundOpacity)}
                    @input=${e=>{this._backgroundOpacity=Number(e.target.value)}}
                  />
                </label>
              `:m}
          <div class="actions">
            <button type="button" class="secondary" @click=${this._close}>取消</button>
            <button type="button" class="primary" @click=${this._save}>保存</button>
          </div>
        </div>
      </div>
    `:m}};wt.styles=w`
    .backdrop {
      position: fixed;
      inset: 0;
      background: rgba(15, 23, 42, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 200;
      padding: 16px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(440px, 100%);
      background: var(--card-background-color);
      border-radius: 16px;
      padding: 20px;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.18);
    }
    h3 {
      margin: 0 0 8px;
      font-size: 18px;
      font-weight: 600;
    }
    .hint {
      margin: 0 0 16px;
      font-size: 13px;
      color: var(--secondary-text-color);
      line-height: 1.45;
    }
    label {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    label.checkbox {
      flex-direction: row;
      align-items: center;
      gap: 8px;
      color: var(--primary-text-color);
    }
    ha-color-picker {
      display: block;
      margin-bottom: 12px;
    }
    .actions {
      display: flex;
      justify-content: flex-end;
      gap: 8px;
      margin-top: 8px;
    }
    button {
      border-radius: 8px;
      padding: 8px 14px;
      font-weight: 600;
      cursor: pointer;
      border: 1px solid var(--divider-color);
      background: #fff;
    }
    button.primary {
      background: var(--primary-color);
      border-color: var(--primary-color);
      color: #fff;
    }
  `;Ci([u({type:Boolean})],wt.prototype,"open",2);Ci([u({attribute:!1})],wt.prototype,"config",2);Ci([u({type:Number})],wt.prototype,"maxColumns",2);Ci([g()],wt.prototype,"_columnSpan",2);Ci([g()],wt.prototype,"_backgroundEnabled",2);Ci([g()],wt.prototype,"_backgroundColor",2);Ci([g()],wt.prototype,"_backgroundOpacity",2);wt=Ci([x("flow-section-editor")],wt);var ix=Object.defineProperty,rx=Object.getOwnPropertyDescriptor,Ye=(e,t,i,r)=>{for(var o=r>1?void 0:r?rx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ix(t,i,o),o};const ox=/^[a-zA-Z0-9_-]+$/,Su=/^[0-9]+$/;function ah(e){const t=e.trim().toLowerCase().normalize("NFKD").replace(/[\u0300-\u036f]/g,"").replace(/[^a-z0-9]+/g,"-").replace(/^-+|-+$/g,"");return t?Su.test(t)?`view-${t}`:t:""}let Se=class extends y{constructor(){super(...arguments),this.open=!1,this.isNew=!1,this.reservedPaths=[],this._title="",this._icon="mdi:view-dashboard",this._path="",this._maxColumns=4,this._densePlacement=!1,this._pathError="",this._suggestedPath=!1,this._backdropDismiss=mr(()=>this._close())}updated(e){if((e.has("open")||e.has("config"))&&this.open){const t=this.config;this._title=(t==null?void 0:t.title)??(this.isNew?"新建视图":""),this._icon=(t==null?void 0:t.icon)||"mdi:view-dashboard",this._path=(t==null?void 0:t.path)??"",this._maxColumns=Math.max(1,Math.min(10,Number(t==null?void 0:t.max_columns)||4)),this._densePlacement=!!(t!=null&&t.dense_section_placement),this._pathError="",this._suggestedPath=!1,this.isNew&&!this._path&&(this._path=this._uniquePath(ah(this._title)||"view"),this._suggestedPath=!0),this._validatePath(this._path)}}_uniquePath(e){const t=new Set(this.reservedPaths);if(!t.has(e)&&e)return e;let i=2,r=`${e||"view"}-${i}`;for(;t.has(r);)i+=1,r=`${e||"view"}-${i}`;return r}_validatePath(e){return e?ox.test(e)?Su.test(e)?(this._pathError="不能是纯数字（会与视图索引冲突）",!1):this.reservedPaths.includes(e)?(this._pathError="该路径已被其他视图占用",!1):(this._pathError="",!0):(this._pathError="仅允许字母、数字、下划线和连字符",!1):(this._pathError="网址路径不能为空",!1)}_onTitleInput(e){const t=e.target.value;if(this._title=t,this.isNew&&this._suggestedPath){const i=ah(t);this._path=this._uniquePath(i||"view"),this._validatePath(this._path)}}_onPathInput(e){this._suggestedPath=!1,this._path=e.target.value.trim(),this._validatePath(this._path)}_close(){this.dispatchEvent(new CustomEvent("close",{bubbles:!0,composed:!0}))}_save(){var t;if(!this._validatePath(this._path))return;const e={...this.config??{sections:[]},title:this._title.trim()||"新建视图",icon:this._icon||"mdi:view-dashboard",path:this._path,max_columns:this._maxColumns,dense_section_placement:this._densePlacement||void 0,sections:((t=this.config)==null?void 0:t.sections)??[]};e.dense_section_placement||delete e.dense_section_placement,this.dispatchEvent(new CustomEvent("save",{bubbles:!0,composed:!0,detail:{config:e,isNew:this.isNew}}))}render(){if(!this.open)return m;const e=!this._pathError&&!!this._path;return h`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog" role="dialog" aria-labelledby="view-editor-title">
          <h3 id="view-editor-title">${this.isNew?"添加视图":"配置视图"}</h3>
          <p class="hint">设置视图标题、图标与网址路径（与 Home Assistant 一致）。</p>

          <label>
            <span>标题</span>
            <input
              type="text"
              .value=${this._title}
              placeholder="例如：客厅"
              @input=${this._onTitleInput}
            />
          </label>

          <ha-icon-picker
            label="图标"
            .value=${this._icon}
            @icon-changed=${t=>{t.stopPropagation(),this._icon=t.detail.value||"mdi:view-dashboard"}}
          ></ha-icon-picker>

          <label>
            <span>网址</span>
            <input
              type="text"
              .value=${this._path}
              placeholder="home"
              @input=${this._onPathInput}
            />
            ${this._pathError?h`<span class="error">${this._pathError}</span>`:h`<span class="helper">该值将成为打开此视图的网址路径的一部分（如 #/${this._path||"…"}）。</span>`}
          </label>

          <label>
            <span>最大列数: ${this._maxColumns}</span>
            <input
              type="range"
              min="1"
              max="10"
              step="1"
              .value=${String(this._maxColumns)}
              @input=${t=>{this._maxColumns=Number(t.target.value)}}
            />
          </label>

          <label class="checkbox">
            <input
              type="checkbox"
              .checked=${this._densePlacement}
              @change=${t=>{this._densePlacement=t.target.checked}}
            />
            <div>
              <div class="switch-label">紧凑分区排列</div>
              <div class="helper">尽量填满空隙放置分区（HA dense_section_placement）。</div>
            </div>
          </label>

          <div class="actions">
            <button type="button" class="secondary" @click=${this._close}>取消</button>
            <button
              type="button"
              class="primary"
              ?disabled=${!e}
              @click=${this._save}
            >
              保存
            </button>
          </div>
        </div>
      </div>
    `}};Se.styles=w`
    .backdrop {
      position: fixed;
      inset: 0;
      background: rgba(15, 23, 42, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 200;
      padding: 16px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(440px, 100%);
      max-height: min(90vh, 720px);
      overflow: auto;
      background: var(--card-background-color, #fff);
      border-radius: 16px;
      padding: 20px;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.18);
      box-sizing: border-box;
    }
    h3 {
      margin: 0 0 8px;
      font-size: 18px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .hint {
      margin: 0 0 16px;
      font-size: 13px;
      color: var(--secondary-text-color);
      line-height: 1.45;
    }
    label {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    label.checkbox {
      flex-direction: row;
      align-items: flex-start;
      gap: 10px;
      color: var(--primary-text-color);
    }
    label.checkbox input {
      margin-top: 3px;
      width: auto;
    }
    .switch-label {
      font-size: 14px;
      font-weight: 500;
      color: var(--primary-text-color);
    }
    input[type="text"] {
      font: inherit;
      font-size: 14px;
      color: var(--primary-text-color);
      border: none;
      border-bottom: 1px solid var(--divider-color);
      border-radius: 4px 4px 0 0;
      background: var(--ha-color-form-background, #f5f5f5);
      padding: 10px 12px;
      outline: none;
    }
    input[type="text"]:focus {
      border-bottom-color: var(--primary-color, #03a9f4);
      border-bottom-width: 2px;
    }
    input[type="range"] {
      width: 100%;
    }
    ha-icon-picker {
      display: block;
      margin-bottom: 12px;
    }
    .helper {
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
    .error {
      font-size: 12px;
      line-height: 1.4;
      color: var(--error-color, #db4437);
    }
    .actions {
      display: flex;
      justify-content: flex-end;
      gap: 8px;
      margin-top: 8px;
    }
    button {
      border-radius: 8px;
      padding: 8px 14px;
      font-weight: 600;
      cursor: pointer;
      border: 1px solid var(--divider-color);
      background: #fff;
      color: var(--primary-text-color);
    }
    button.primary {
      background: var(--primary-color, #03a9f4);
      border-color: var(--primary-color, #03a9f4);
      color: #fff;
    }
    button.primary:disabled {
      opacity: 0.45;
      cursor: not-allowed;
    }
  `;Ye([u({type:Boolean})],Se.prototype,"open",2);Ye([u({type:Boolean})],Se.prototype,"isNew",2);Ye([u({attribute:!1})],Se.prototype,"config",2);Ye([u({attribute:!1})],Se.prototype,"reservedPaths",2);Ye([g()],Se.prototype,"_title",2);Ye([g()],Se.prototype,"_icon",2);Ye([g()],Se.prototype,"_path",2);Ye([g()],Se.prototype,"_maxColumns",2);Ye([g()],Se.prototype,"_densePlacement",2);Ye([g()],Se.prototype,"_pathError",2);Ye([g()],Se.prototype,"_suggestedPath",2);Se=Ye([x("flow-view-editor")],Se);async function nx(){try{const e=await fetch("/api/info/app");if(!e.ok)return{name:"Flow",version:"未知"};const t=await e.json();return t.ok?{name:t.name?String(t.name):"Flow",version:t.version?String(t.version):"未知"}:{name:"Flow",version:"未知"}}catch{return{name:"Flow",version:"获取失败"}}}async function sx(){try{const e=await fetch("/api/info/current_flow");if(!e.ok)return"获取失败";const t=await e.json();return t.ok&&t.filename?String(t.filename):"无项目运行"}catch{return"连接异常"}}function Eu(e,t,i){return new Promise((r,o)=>{if(!t||typeof t.size!="number"){o(new Error("invalid_file"));return}const n=new XMLHttpRequest;n.open("POST",e,!0),n.setRequestHeader("Content-Type","application/octet-stream"),n.upload.onprogress=s=>{i&&(s.lengthComputable&&s.total>0?i(Math.round(s.loaded/s.total*100)):t.size>0&&i(Math.min(99,Math.round(s.loaded/t.size*100))))},n.onload=()=>{const s=n.responseText||"";let a=null;try{a=JSON.parse(s)}catch{a=null}if(n.status>=200&&n.status<300&&(a!=null&&a.ok)){i==null||i(100),r({ok:!0,path:a.path||""});return}o(new Error((a==null?void 0:a.error)??`${n.status} ${s}`))},n.onerror=()=>o(new Error("network_error")),n.onabort=()=>o(new Error("aborted")),n.send(t)})}async function ax(){const e=await fetch("/api/download/current_flow");if(!e.ok){const n=await e.json().catch(()=>({}));throw new Error(n.error||e.statusText)}const t=await e.blob(),i=e.headers.get("Content-Disposition")||"";let r="project.flow";const o=/filename="([^"]*)"/.exec(i);return o!=null&&o[1]&&(r=o[1]),{filename:r,blob:t}}const lx="video/*,audio/*,image/*,.mp4,.mov,.avi,.mkv,.mp3,.wav,.png,.jpg,.jpeg,.gif",Pu="ns_setting_auth_ts",cx=10*60*1e3;function lh(){try{sessionStorage.setItem(Pu,String(Date.now()))}catch{}}function dx(){try{const e=Number(sessionStorage.getItem(Pu)||0);return e>0&&Date.now()-e<cx}catch{return!1}}async function Au(e){try{const t=await fetch("/api/auth/setting",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({password:e||""})}),i=await t.json().catch(()=>null);return t.ok&&(i!=null&&i.ok)?"ok":"deny"}catch{return"offline"}}async function hx(e){const t=await Au(e);return t==="ok"||t==="offline"}var px=Object.defineProperty,ux=Object.getOwnPropertyDescriptor,re=(e,t,i,r)=>{for(var o=r>1?void 0:r?ux(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&px(t,i,o),o};const fx=2;let J=class extends y{constructor(){super(...arguments),this._loading=!0,this._authed=!1,this._passwordRequired=!0,this._authPassword="",this._authError="",this._authLoading=!1,this._layoutEditEnabled=!1,this._appName="Flow",this._appVersion="…",this._currentFlowName="检测中...",this._flowFile=null,this._flowStatus="",this._downloadStatus="",this._flowUploading=!1,this._downloading=!1,this._flowProgress={show:!1,percent:0},this._columnCount=1}connectedCallback(){super.connectedCallback(),this._resizeObserver=new ResizeObserver(e=>{var n;const t=((n=e[0])==null?void 0:n.contentRect.width)??0;if(!t)return;const i=320,r=32,o=Math.max(1,Math.min(fx,Math.floor((t+r)/(i+r))));o!==this._columnCount&&(this._columnCount=o)}),this._resizeObserver.observe(this),this._layoutEditEnabled=Sp(),this._loadMeta()}disconnectedCallback(){var e;(e=this._resizeObserver)==null||e.disconnect(),super.disconnectedCallback()}_gridStyle(){return`--column-count: ${this._columnCount}`}async _loadMeta(){this._loading=!0;try{const e=await nx();this._appName=e.name,this._appVersion=e.version,this._currentFlowName=await sx(),await this._refreshAuthState()}finally{this._loading=!1}}async _refreshAuthState(){if(dx()){this._authed=!0;return}const e=await Au("");if(e==="ok"||e==="offline"){lh(),this._authed=!0,this._passwordRequired=!1;return}this._authed=!1,this._passwordRequired=!0}async _submitAuth(){this._authLoading=!0,this._authError="";try{if(await hx(this._authPassword)){lh(),this._authed=!0,this._authPassword="";return}this._authError="密码错误，请重试"}finally{this._authLoading=!1}}_onAuthKeydown(e){e.key==="Enter"&&this._submitAuth()}_toggleLayoutEdit(e){if(!this._authed)return;const t=e.target.checked;this._layoutEditEnabled=t,Hv(t)}_onFlowInput(e){var r;const t=e.target,i=(r=t.files)==null?void 0:r[0];this._flowFile=i?{file:i,status:"ready"}:null,t.value=""}async _uploadFlow(){const e=this._flowFile,t=e==null?void 0:e.file;if(!t){this._flowStatus="请选择.flow文件";return}if(!t.name.toLowerCase().endsWith(".flow")){this._flowStatus="文件扩展名必须为 .flow";return}this._flowUploading=!0,this._flowProgress={show:!0,percent:0},this._flowStatus="开始上传...";try{const i=`/api/upload/flow?filename=${encodeURIComponent(t.name)}`,r=await Eu(i,t,o=>{this._flowProgress={show:!0,percent:o}});this._flowProgress={show:!0,percent:100},this._flowStatus=`上传成功：${r.path}`,this._flowFile={file:t,status:"success"}}catch(i){this._flowStatus=`上传失败：${i instanceof Error?i.message:i}`,this._flowFile={file:t,status:"fail"}}finally{this._flowUploading=!1}}async _downloadFlow(){this._downloadStatus="正在请求...",this._downloading=!0;try{const{filename:e,blob:t}=await ax(),i=URL.createObjectURL(t),r=document.createElement("a");r.href=i,r.download=e,document.body.appendChild(r),r.click(),r.remove(),URL.revokeObjectURL(i),this._downloadStatus="下载完成"}catch(e){this._downloadStatus=`下载失败：${e instanceof Error?e.message:e}`}finally{this._downloading=!1}}_renderAuthCard(){return h`
      <div class="grid-slot">
        <ha-card>
          <div class="card-body">
            <h2 class="card-title">访问验证</h2>
            ${this._authed?h`
                  <p class="help auth-ok">已通过验证，可使用下方设置项。</p>
                  <span class="tag success">已解锁</span>
                `:h`
                  <p class="help">请输入设置密码以解锁上传、下载与编辑模式等操作。</p>
                  <input
                    class="text-input"
                    type="password"
                    .value=${this._authPassword}
                    placeholder="请输入设置页面密码"
                    autocomplete="current-password"
                    ?disabled=${this._authLoading}
                    @input=${e=>{this._authPassword=e.target.value}}
                    @keydown=${this._onAuthKeydown}
                  />
                  ${this._authError?h`<div class="alert error">${this._authError}</div>`:m}
                  <div class="actions">
                    <button
                      type="button"
                      class="btn primary"
                      ?disabled=${this._authLoading}
                      @click=${this._submitAuth}
                    >
                      ${this._authLoading?"验证中...":"验证并解锁"}
                    </button>
                  </div>
                `}
          </div>
        </ha-card>
      </div>
    `}_renderLockedSlot(e){return h`
      <div class="grid-slot ${this._authed?"":"locked"}">
        <div class="card-lock-wrap" ?inert=${!this._authed}>${e}</div>
      </div>
    `}_renderMain(){return h`
      <div class="wrapper" style=${this._gridStyle()}>
        <div class="grid">
          ${this._passwordRequired?this._renderAuthCard():m}

          ${this._renderLockedSlot(h`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">编辑模式</h2>
                <p class="help">开启后，仪表盘右上角会出现编辑按钮，可修改布局与卡片。</p>
                <label class="switch-row">
                  <span>允许编辑仪表盘</span>
                  <input
                    type="checkbox"
                    .checked=${this._layoutEditEnabled}
                    ?disabled=${!this._authed}
                    @change=${this._toggleLayoutEdit}
                  />
                </label>
              </div>
            </ha-card>
          `)}

          ${this._renderLockedSlot(h`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">上传项目文件（.flow）</h2>
                <div class="flow-upload-row">
                  <label class="btn primary file-label">
                    选择文件
                    <input
                      type="file"
                      accept=".flow"
                      ?disabled=${this._flowUploading}
                      @change=${this._onFlowInput}
                    />
                  </label>
                  <button
                    type="button"
                    class="btn success"
                    ?disabled=${this._flowUploading}
                    @click=${this._uploadFlow}
                  >
                    ${this._flowUploading?"上传中...":"上传到服务器"}
                  </button>
                </div>
                <p class="upload-tip">仅允许单个 .flow 项目文件</p>
                ${this._flowFile?h`<div class="file-chip ${this._flowFile.status??""}">${this._flowFile.file.name}</div>`:m}
                ${this._flowStatus?h`<div class="status block">${this._flowStatus}</div>`:m}
                ${this._flowProgress.show?h`
                      <div class="progress-wrap">
                        <div class="progress-bar">
                          <div
                            class="progress-fill success"
                            style=${`width:${this._flowProgress.percent}%`}
                          ></div>
                        </div>
                      </div>
                    `:m}
              </div>
            </ha-card>
          `)}

          ${this._renderLockedSlot(h`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">下载当前项目</h2>
                <p class="help">下载当前正在运行的项目文件</p>
                <p class="help">
                  当前文件：<span class="tag">${this._currentFlowName}</span>
                </p>
                <div class="actions">
                  <button
                    type="button"
                    class="btn success"
                    ?disabled=${this._downloading}
                    @click=${this._downloadFlow}
                  >
                    ${this._downloading?"下载中...":"下载文件"}
                  </button>
                  <span class="status">${this._downloadStatus}</span>
                </div>
              </div>
            </ha-card>
          `)}

          ${this._renderLockedSlot(h`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">关于软件</h2>
                <div class="about-row">
                  <div class="about-meta">
                    <strong>${this._appName}</strong>
                    <span>版本 ${this._appVersion}</span>
                  </div>
                  <span class="tag success">v${this._appVersion}</span>
                </div>
              </div>
            </ha-card>
          `)}
        </div>
      </div>
    `}render(){return h`
      <div class="page">
        ${this._loading?h`<div class="wrapper" style=${this._gridStyle()}>
              <div class="grid">
                <div class="grid-slot">
                  <ha-card>
                    <div class="card-body">
                      <div class="checking">加载中...</div>
                    </div>
                  </ha-card>
                </div>
              </div>
            </div>`:this._renderMain()}
      </div>
    `}};J.styles=w`
    :host {
      --row-gap: var(--ha-view-sections-row-gap, 16px);
      --column-gap: var(--ha-view-sections-column-gap, 24px);
      --column-max-width: var(--ha-view-sections-column-max-width, 500px);
      --column-min-width: var(--ha-view-sections-column-min-width, 320px);
      display: block;
      flex: 1;
      min-height: 0;
      overflow: auto;
    }
    .page {
      min-height: 100%;
      background: var(--primary-background-color, #e8edf1);
      color: var(--primary-text-color);
      font-family: var(--ha-font-family, system-ui, sans-serif);
    }
    .wrapper {
      padding: 0 var(--column-gap);
      box-sizing: content-box;
      margin: var(--ha-view-sections-extra-top-margin, 16px) auto 0;
      max-width: calc(
        var(--column-count) * var(--column-max-width) +
          (var(--column-count) - 1) * var(--column-gap)
      );
    }
    /* 多列瀑布流：按高度自动补齐，避免网格行对齐留下空白 */
    .grid {
      column-count: var(--column-count);
      column-gap: var(--column-gap);
      column-fill: balance;
      padding: var(--row-gap) 0 calc(var(--row-gap) * 2);
    }
    .grid-slot {
      display: inline-block;
      width: 100%;
      min-width: 0;
      margin: 0 0 var(--row-gap);
      break-inside: avoid;
      page-break-inside: avoid;
      -webkit-column-break-inside: avoid;
    }
    .grid-slot.locked .card-lock-wrap {
      opacity: 0.42;
      pointer-events: none;
      user-select: none;
      filter: grayscale(0.35);
    }
    .auth-ok {
      margin-bottom: 6px;
    }
    .text-input {
      width: 100%;
      box-sizing: border-box;
      height: 36px;
      border: 1px solid #d1d5db;
      border-radius: 8px;
      padding: 0 12px;
      font: inherit;
    }
    .text-input:focus {
      outline: none;
      border-color: #03a9f4;
      box-shadow: 0 0 0 2px rgba(3, 169, 244, 0.15);
    }
    .alert {
      margin-top: 8px;
      padding: 8px 10px;
      border-radius: 8px;
      font-size: 13px;
    }
    .alert.error {
      background: #fef2f2;
      color: #b91c1c;
      border: 1px solid #fecaca;
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 4px 0 0;
      font-size: 14px;
      cursor: pointer;
    }
    .switch-row input {
      width: 18px;
      height: 18px;
      cursor: pointer;
      accent-color: #03a9f4;
    }
    .card-body {
      padding: 14px 16px;
    }
    .card-title {
      margin: 0 0 8px;
      font-size: 15px;
      font-weight: 600;
    }
    .checking {
      text-align: center;
      padding: 12px 0;
      color: #64748b;
    }
    .help {
      margin: 0 0 10px;
      color: var(--secondary-text-color);
      font-size: 13px;
      line-height: 1.45;
    }
    .help.tight {
      margin-bottom: 8px;
      font-size: 12px;
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 10px;
    }
    .status {
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
      word-break: break-all;
    }
    .status.block {
      margin-top: 8px;
      display: block;
    }
    .btn {
      border: 1px solid #d1d5db;
      background: #fff;
      color: #1f2937;
      border-radius: 8px;
      height: 36px;
      padding: 0 14px;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
    }
    .btn:disabled {
      opacity: 0.6;
      cursor: default;
    }
    .btn.primary {
      background: #03a9f4;
      border-color: #03a9f4;
      color: #fff;
    }
    .btn.success {
      background: #22c55e;
      border-color: #22c55e;
      color: #fff;
    }
    .dropzone {
      position: relative;
      border: 1px dashed #c0c4cc;
      border-radius: 8px;
      background: #fafafa;
      padding: 16px 12px;
      text-align: center;
      color: #606266;
    }
    .dropzone.dragover {
      border-color: #03a9f4;
      background: rgba(3, 169, 244, 0.04);
    }
    .dropzone em {
      color: #03a9f4;
      font-style: normal;
    }
    .file-input {
      position: absolute;
      inset: 0;
      opacity: 0;
      cursor: pointer;
    }
    .upload-icon {
      font-size: 36px;
      color: #c0c4cc;
      margin-bottom: 4px;
      line-height: 1;
    }
    .upload-icon svg {
      width: 36px;
      height: 36px;
      display: block;
      margin: 0 auto;
    }
    .upload-tip {
      margin: 8px 0 0;
      font-size: 12px;
      color: #909399;
    }
    .file-list {
      list-style: none;
      margin: 12px 0 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .file-item {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      padding: 8px 10px;
      border-radius: 8px;
      background: #f8fafc;
      font-size: 13px;
    }
    .file-item.success {
      color: #15803d;
    }
    .file-item.fail {
      color: #b91c1c;
    }
    .link-btn {
      border: none;
      background: transparent;
      color: #03a9f4;
      cursor: pointer;
      font-size: 12px;
      padding: 0;
    }
    .flow-upload-row {
      display: flex;
      align-items: center;
      gap: 12px;
      flex-wrap: wrap;
    }
    .file-label {
      position: relative;
      overflow: hidden;
      display: inline-flex;
      align-items: center;
      justify-content: center;
    }
    .file-label input {
      position: absolute;
      inset: 0;
      opacity: 0;
      cursor: pointer;
    }
    .file-chip {
      display: inline-block;
      margin-top: 8px;
      padding: 6px 10px;
      border-radius: 8px;
      background: #f1f5f9;
      font-size: 13px;
    }
    .file-chip.success {
      background: #dcfce7;
      color: #15803d;
    }
    .file-chip.fail {
      background: #fee2e2;
      color: #b91c1c;
    }
    .tag {
      display: inline-block;
      padding: 2px 8px;
      border-radius: 6px;
      background: #f1f5f9;
      color: #475569;
      font-size: 12px;
    }
    .tag.success {
      background: #dcfce7;
      color: #15803d;
    }
    .about-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      flex-wrap: wrap;
    }
    .about-meta {
      font-size: 14px;
    }
    .about-meta span {
      margin-left: 8px;
      color: #64748b;
    }
    .progress-wrap {
      margin-top: 12px;
    }
    .progress-bar {
      height: 8px;
      background: #e5e7eb;
      border-radius: 999px;
      overflow: hidden;
    }
    .progress-fill {
      height: 100%;
      background: #03a9f4;
      transition: width 0.2s ease;
    }
    .progress-fill.success {
      background: #22c55e;
    }
    .progress-label {
      display: block;
      margin-top: 6px;
      font-size: 12px;
      color: var(--secondary-text-color);
    }

    @media (max-width: 600px) {
      .grid {
        grid-template-columns: 1fr;
      }
    }
  `;re([u({attribute:!1})],J.prototype,"flow",2);re([g()],J.prototype,"_loading",2);re([g()],J.prototype,"_authed",2);re([g()],J.prototype,"_passwordRequired",2);re([g()],J.prototype,"_authPassword",2);re([g()],J.prototype,"_authError",2);re([g()],J.prototype,"_authLoading",2);re([g()],J.prototype,"_layoutEditEnabled",2);re([g()],J.prototype,"_appName",2);re([g()],J.prototype,"_appVersion",2);re([g()],J.prototype,"_currentFlowName",2);re([g()],J.prototype,"_flowFile",2);re([g()],J.prototype,"_flowStatus",2);re([g()],J.prototype,"_downloadStatus",2);re([g()],J.prototype,"_flowUploading",2);re([g()],J.prototype,"_downloading",2);re([g()],J.prototype,"_flowProgress",2);re([g()],J.prototype,"_columnCount",2);J=re([x("flow-settings-panel")],J);var gx=Object.defineProperty,mx=Object.getOwnPropertyDescriptor,Si=(e,t,i,r)=>{for(var o=r>1?void 0:r?mx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&gx(t,i,o),o};function ch(e){return e.access!=="read"}function bx(e,t){const i=e.toLowerCase();if(i.includes("/trigger")||i.endsWith("trigger"))return!0;const r=t==null?void 0:t.states[e];return r?Cl(r):!0}let $t=class extends y{constructor(){super(...arguments),this._actions=[],this._loading=!0,this._error="",this._busyEntity="",this._testingEntity="",this._testHint="",this._onActionsChanged=()=>{this._load()}}connectedCallback(){super.connectedCallback(),document.addEventListener("flow-actions-changed",this._onActionsChanged),this._load()}disconnectedCallback(){document.removeEventListener("flow-actions-changed",this._onActionsChanged),super.disconnectedCallback()}async _load(){this._loading=!0,this._error="";try{const e=await Qp();this._actions=[...e].sort((i,r)=>pe(i).localeCompare(pe(r),"zh-CN"));const t=this._actions.map(i=>i.entity).filter(Boolean);t.length&&this.flow&&this.flow.connection.trackAddresses(t)}catch{this._error="加载动作库失败",this._actions=[]}finally{this._loading=!1}}async _remove(e){const t=pe(e),i=e.used?"该动作仍被页面卡片引用，删除后相关控件将失效。":"删除后可从节点重新导出，或手动重新添加。";if(!await il({title:"删除动作",text:`确定删除「${t}」（${e.entity}）？ ${i}`,confirmText:"删除",cancelText:"取消",destructive:!0}))return;this._busyEntity=e.entity,this._error="";const o=await x0(e.entity);if(this._busyEntity="",!o){this._error=`删除失败：${t}`;return}await this._load()}async _test(e){if(!this.flow||!ch(e))return;const t=pe(e),i=bx(e.entity,this.flow);this._testingEntity=e.entity,this._testHint="",this._error="";try{await this.flow.callService(e.entity,i);const r=typeof i=="boolean"?i?"true":"false":String(i);this._testHint=`已测试「${t}」→ ${r}`}catch{this._error=`测试失败：${t}`}finally{this._testingEntity=""}}_renderRow(e){const t=this._busyEntity===e.entity,i=this._testingEntity===e.entity,r=pe(e),o=ch(e),n=!!(this._busyEntity||this._testingEntity);return h`
      <div class="row">
        <div class="meta">
          <span class="name" title=${r}>${r}</span>
          <span class="entity" title=${e.entity}>${e.entity}</span>
        </div>
        <div class="tags">
          <span class="tag">${_0(e.access)}</span>
          ${e.used?h`<span class="tag used">使用中</span>`:h`<span class="tag idle">未使用</span>`}
        </div>
        <div class="ops">
          <button
            type="button"
            class="test"
            title=${o?"发送测试值":"只读动作不可测试"}
            aria-label=${`测试 ${r}`}
            ?disabled=${!o||n}
            @click=${()=>void this._test(e)}
          >
            ${i?"…":"测试"}
          </button>
          <button
            type="button"
            class="remove"
            title="删除"
            aria-label=${`删除 ${r}`}
            ?disabled=${n}
            @click=${()=>void this._remove(e)}
          >
            ${t?"…":"×"}
          </button>
        </div>
      </div>
    `}render(){return h`
      <div class="section-head">
        <span class="section-title">已登记动作</span>
        ${this._loading?m:h`<span class="count">${this._actions.length}</span>`}
      </div>
      ${this._error?h`<p class="error">${this._error}</p>`:m}
      ${this._testHint?h`<p class="hint">${this._testHint}</p>`:m}
      ${this._loading?h`<p class="empty">加载中…</p>`:this._actions.length===0?h`<p class="empty">暂无动作。可通过节点右键导出，或在「手动添加」中登记。</p>`:h`<div class="list">${this._actions.map(e=>this._renderRow(e))}</div>`}
    `}};$t.styles=w`
    :host {
      display: block;
    }
    .section-head {
      display: flex;
      align-items: center;
      gap: 6px;
      margin-bottom: 8px;
    }
    .section-title {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .count {
      font-size: 11px;
      color: var(--secondary-text-color);
      background: color-mix(in srgb, var(--divider-color) 70%, transparent);
      border-radius: 999px;
      padding: 0 6px;
      line-height: 18px;
    }
    .empty {
      margin: 0;
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
    .error {
      margin: 0 0 6px;
      font-size: 12px;
      color: var(--error-color, #db4437);
    }
    .hint {
      margin: 0 0 6px;
      font-size: 12px;
      color: var(--success-color, #0f9d58);
    }
    .list {
      max-height: min(52vh, 420px);
      overflow-y: auto;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      background: #fafbfc;
    }
    .row {
      display: grid;
      grid-template-columns: minmax(0, 1fr) auto auto;
      align-items: center;
      gap: 8px;
      min-height: 36px;
      padding: 4px 8px;
      border-bottom: 1px solid var(--divider-color);
    }
    .row:last-child {
      border-bottom: none;
    }
    .meta {
      min-width: 0;
      display: flex;
      align-items: baseline;
      gap: 8px;
    }
    .name {
      flex: 0 1 auto;
      max-width: 42%;
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .entity {
      flex: 1 1 auto;
      min-width: 0;
      font-size: 11px;
      color: var(--secondary-text-color);
      font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .tags {
      display: flex;
      align-items: center;
      gap: 4px;
      flex-shrink: 0;
    }
    .tag {
      font-size: 10px;
      line-height: 16px;
      padding: 0 6px;
      border-radius: 999px;
      background: color-mix(in srgb, var(--divider-color) 65%, transparent);
      color: var(--secondary-text-color);
      white-space: nowrap;
    }
    .tag.used {
      background: color-mix(in srgb, var(--primary-color, #03a9f4) 18%, transparent);
      color: var(--primary-color, #03a9f4);
    }
    .tag.idle {
      background: color-mix(in srgb, #94a3b8 20%, transparent);
      color: #64748b;
    }
    .ops {
      display: flex;
      align-items: center;
      gap: 4px;
      flex-shrink: 0;
    }
    .test,
    .remove {
      flex: 0 0 auto;
      height: 26px;
      border: 1px solid var(--divider-color);
      border-radius: 6px;
      background: #fff;
      font: inherit;
      line-height: 1;
      cursor: pointer;
      padding: 0;
    }
    .test {
      min-width: 40px;
      padding: 0 8px;
      font-size: 12px;
      font-weight: 600;
      color: var(--primary-color, #03a9f4);
    }
    .test:hover:not(:disabled) {
      background: color-mix(in srgb, var(--primary-color, #03a9f4) 10%, #fff);
    }
    .remove {
      width: 26px;
      color: var(--error-color, #db4437);
      font-size: 16px;
    }
    .remove:hover:not(:disabled) {
      background: color-mix(in srgb, var(--error-color, #db4437) 10%, #fff);
    }
    .test:disabled,
    .remove:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
  `;Si([cr({context:vi,subscribe:!0}),u({attribute:!1})],$t.prototype,"flow",2);Si([g()],$t.prototype,"_actions",2);Si([g()],$t.prototype,"_loading",2);Si([g()],$t.prototype,"_error",2);Si([g()],$t.prototype,"_busyEntity",2);Si([g()],$t.prototype,"_testingEntity",2);Si([g()],$t.prototype,"_testHint",2);$t=Si([x("flow-action-library")],$t);var vx=Object.defineProperty,_x=Object.getOwnPropertyDescriptor,po=(e,t,i,r)=>{for(var o=r>1?void 0:r?_x(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&vx(t,i,o),o};let gi=class extends y{constructor(){super(...arguments),this._entity="",this._name="",this._error="",this._adding=!1,this._success=""}_reset(){this._entity="",this._name="",this._error=""}async _submit(){const e=this._entity.trim();if(!e){this._error="请输入控制地址";return}if(!e.startsWith("/")){this._error="控制地址应以 / 开头";return}this._adding=!0,this._error="",this._success="";const t=this._name.trim()||e,i=await y0(e,t);if(this._adding=!1,!i){this._error="添加失败，请检查地址是否有效";return}this._success=`已添加：${pe(i)}`,this._reset()}render(){return h`
      <p class="help">无节点右键导出时，可在此手动登记控制地址。</p>
      <div class="form">
        <label class="field">
          <span class="label">控制地址</span>
          <input
            type="text"
            .value=${this._entity}
            placeholder="/dataflow/0/mix"
            ?disabled=${this._adding}
            @input=${e=>{this._entity=e.target.value,this._error="",this._success=""}}
          />
        </label>
        <label class="field">
          <span class="label">显示名称</span>
          <input
            type="text"
            .value=${this._name}
            placeholder="留空则用地址"
            ?disabled=${this._adding}
            @input=${e=>{this._name=e.target.value}}
          />
        </label>
        <div class="actions">
          <button
            type="button"
            class="btn primary"
            ?disabled=${this._adding}
            @click=${()=>void this._submit()}
          >
            ${this._adding?"添加中…":"添加"}
          </button>
        </div>
      </div>
      ${this._error?h`<p class="error">${this._error}</p>`:m}
      ${this._success?h`<p class="success">${this._success}</p>`:m}
    `}};gi.styles=w`
    :host {
      display: block;
    }
    .help {
      margin: 0 0 8px;
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
    .form {
      display: grid;
      grid-template-columns: minmax(0, 1.3fr) minmax(0, 1fr) auto;
      gap: 8px;
      align-items: end;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 4px;
      min-width: 0;
    }
    .label {
      font-size: 12px;
      color: var(--secondary-text-color);
    }
    input {
      width: 100%;
      box-sizing: border-box;
      height: 32px;
      font: inherit;
      font-size: 13px;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 0 8px;
      color: var(--primary-text-color);
      background: #fff;
    }
    .error,
    .success {
      margin: 6px 0 0;
      font-size: 12px;
    }
    .error {
      color: var(--error-color, #db4437);
    }
    .success {
      color: var(--success-color, #0f9d58);
    }
    .actions {
      display: flex;
      align-items: flex-end;
    }
    .btn {
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      background: #fff;
      color: var(--primary-text-color);
      font: inherit;
      font-size: 13px;
      font-weight: 600;
      height: 32px;
      padding: 0 12px;
      cursor: pointer;
      white-space: nowrap;
    }
    .btn.primary {
      border-color: var(--primary-color);
      background: var(--primary-color);
      color: #fff;
    }
    .btn:disabled {
      opacity: 0.6;
      cursor: not-allowed;
    }
    @media (max-width: 520px) {
      .form {
        grid-template-columns: 1fr;
      }
      .actions {
        justify-content: flex-start;
      }
    }
  `;po([g()],gi.prototype,"_entity",2);po([g()],gi.prototype,"_name",2);po([g()],gi.prototype,"_error",2);po([g()],gi.prototype,"_adding",2);po([g()],gi.prototype,"_success",2);gi=po([x("flow-manual-action-form")],gi);var yx=Object.defineProperty,xx=Object.getOwnPropertyDescriptor,Tu=(e,t,i,r)=>{for(var o=r>1?void 0:r?xx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&yx(t,i,o),o};let Hs=class extends y{render(){return h`
      <div class="page">
        <div class="wrapper">
          <ha-card>
            <div class="card-body">
              <h2 class="card-title">动作库</h2>
              <p class="help">管理已发布到网页的控制地址，可删除错误或不需要的动作；点击「测试」可向地址发送一次测试值。</p>
              <flow-action-library .flow=${this.flow}></flow-action-library>
            </div>
          </ha-card>

          <ha-card>
            <div class="card-body">
              <h2 class="card-title">手动添加</h2>
              <flow-manual-action-form></flow-manual-action-form>
            </div>
          </ha-card>
        </div>
      </div>
    `}};Hs.styles=w`
    :host {
      display: block;
      flex: 1;
      min-height: 0;
      overflow: auto;
    }
    .page {
      min-height: 100%;
      background: var(--primary-background-color, #e8edf1);
      color: var(--primary-text-color);
      font-family: var(--ha-font-family, system-ui, sans-serif);
    }
    .wrapper {
      max-width: 960px;
      margin: 16px auto 0;
      padding: 0 24px 32px;
      box-sizing: border-box;
      display: flex;
      flex-direction: row;
      align-items: stretch;
      gap: 16px;
    }
    .wrapper > ha-card {
      flex: 1 1 0;
      min-width: 0;
    }
    @media (max-width: 720px) {
      .wrapper {
        flex-direction: column;
      }
    }
    .card-body {
      padding: 16px 18px;
    }
    .card-title {
      margin: 0 0 8px;
      font-size: 16px;
      font-weight: 600;
    }
    .help {
      margin: 0 0 12px;
      color: var(--secondary-text-color);
      font-size: 13px;
      line-height: 1.45;
    }
  `;Tu([u({attribute:!1})],Hs.prototype,"flow",2);Hs=Tu([x("flow-actions-panel")],Hs);async function dh(e){const t=new URLSearchParams,i=(e==null?void 0:e.limit)??300;t.set("limit",String(i)),e!=null&&e.level&&e.level!=="All"&&t.set("level",e.level),e!=null&&e.since&&e.since>0&&t.set("since",String(e.since));try{const o=await(await fetch(`/api/logs/tail?${t.toString()}`)).json();return o.ok===!1?{ok:!1,items:[],error:o.error||"加载失败"}:{ok:!0,items:Array.isArray(o.items)?o.items:[]}}catch{return{ok:!1,items:[],error:"无法连接 Flow 日志服务"}}}function wx(e){switch(e){case"Debug":return"level-debug";case"Warn":return"level-warn";case"Critical":case"Fatal":return"level-critical";default:return"level-info"}}async function $x(){try{const t=await(await fetch("/api/logs/files")).json();return t.ok===!1?{ok:!1,items:[],error:t.error||"加载失败"}:{ok:!0,items:Array.isArray(t.items)?t.items:[]}}catch{return{ok:!1,items:[],error:"无法连接 Flow 日志服务"}}}async function kx(e){try{const t=new URLSearchParams({file:e}),i=await fetch(`/api/logs/download?${t.toString()}`);if(!i.ok)return!1;const r=await i.blob(),o=URL.createObjectURL(r),n=document.createElement("a");return n.href=o,n.download=e,n.click(),URL.revokeObjectURL(o),!0}catch{return!1}}function Cx(e){return!Number.isFinite(e)||e<=0?"0 B":e<1024?`${e} B`:e<1024*1024?`${(e/1024).toFixed(1)} KB`:`${(e/(1024*1024)).toFixed(1)} MB`}function Sx(e){if(!e)return"—";const t=new Date(e);return Number.isNaN(t.getTime())?e:t.toLocaleString("zh-CN",{year:"numeric",month:"2-digit",day:"2-digit",hour:"2-digit",minute:"2-digit"})}var Ex=Object.defineProperty,Px=Object.getOwnPropertyDescriptor,An=(e,t,i,r)=>{for(var o=r>1?void 0:r?Px(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ex(t,i,o),o};let ar=class extends y{constructor(){super(...arguments),this._files=[],this._loading=!0,this._error="",this._downloading=""}connectedCallback(){super.connectedCallback(),this._load()}async _load(){this._loading=!0,this._error="";const e=await $x();if(this._loading=!1,!e.ok){this._error=e.error||"加载失败",this._files=[];return}this._files=e.items}async _download(e){this._downloading=e.name,this._error="";const t=await kx(e.name);this._downloading="",t||(this._error=`下载失败：${e.name}`)}render(){return h`
      <ha-card>
        <div class="card-body">
          <div class="toolbar">
            <div>
              <h2 class="card-title">历史记录</h2>
              <p class="help">下载磁盘上按天保存的日志文件（Documents/Flow/Logs）。</p>
            </div>
            <button type="button" class="ghost" ?disabled=${this._loading} @click=${()=>void this._load()}>
              刷新列表
            </button>
          </div>

          ${this._error?h`<p class="error">${this._error}</p>`:m}

          ${this._loading?h`<p class="empty">加载中…</p>`:this._files.length===0?h`<p class="empty">暂无历史日志文件</p>`:h`
                  <ul class="file-list">
                    ${this._files.map(e=>h`
                        <li class="file-row">
                          <div class="file-meta">
                            <span class="file-name">${e.name}</span>
                            <span class="file-sub">
                              ${Sx(e.modified)} · ${Cx(e.size)}
                            </span>
                          </div>
                          <button
                            type="button"
                            class="download"
                            ?disabled=${this._downloading===e.name}
                            @click=${()=>void this._download(e)}
                          >
                            ${this._downloading===e.name?"下载中…":"下载"}
                          </button>
                        </li>
                      `)}
                  </ul>
                `}
        </div>
      </ha-card>
    `}};ar.styles=w`
    :host {
      display: flex;
      flex: 1;
      min-height: 0;
      min-width: 0;
      max-height: 100%;
      overflow: hidden;
    }
    @media (max-width: 870px) {
      :host {
        flex: none;
        max-height: none;
        height: 100%;
        min-height: 240px;
      }
    }
    ha-card {
      flex: 1;
      min-height: 0;
      max-height: 100%;
      overflow: hidden;
      display: flex;
      flex-direction: column;
    }
    .card-body {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      padding: 16px 18px;
      overflow: hidden;
    }
    .toolbar {
      display: flex;
      flex-wrap: wrap;
      align-items: flex-start;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 8px;
      flex-shrink: 0;
    }
    .card-title {
      margin: 0 0 4px;
      font-size: 16px;
      font-weight: 600;
    }
    .help {
      margin: 0;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
      font-size: 13px;
      line-height: 1.45;
    }
    button.ghost,
    button.download {
      padding: 6px 12px;
      border-radius: 8px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      background: var(--card-background-color, #fff);
      font-size: 13px;
      cursor: pointer;
      color: var(--primary-text-color, rgba(0, 0, 0, 0.87));
    }
    button.download {
      min-width: 72px;
      border-color: rgba(3, 169, 244, 0.35);
      color: var(--primary-color, #0288d1);
      background: rgba(3, 169, 244, 0.06);
    }
    button:disabled {
      opacity: 0.6;
      cursor: default;
    }
    .error {
      color: var(--error-color, #c62828);
      font-size: 13px;
      margin: 0 0 8px;
    }
    .empty {
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
      margin: 12px 0 0;
      font-size: 13px;
    }
    .file-list {
      list-style: none;
      margin: 0;
      padding: 0;
      flex: 1;
      min-height: 0;
      overflow: auto;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .file-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 10px 12px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.08));
      border-radius: 10px;
      background: var(--secondary-background-color, #f7f9fb);
    }
    .file-meta {
      min-width: 0;
      display: flex;
      flex-direction: column;
      gap: 2px;
    }
    .file-name {
      font-family: ui-monospace, "Cascadia Code", "Consolas", monospace;
      font-size: 13px;
      word-break: break-all;
    }
    .file-sub {
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.54));
    }
  `;An([g()],ar.prototype,"_files",2);An([g()],ar.prototype,"_loading",2);An([g()],ar.prototype,"_error",2);An([g()],ar.prototype,"_downloading",2);ar=An([x("flow-log-history-card")],ar);var Ax=Object.defineProperty,Tx=Object.getOwnPropertyDescriptor,Wt=(e,t,i,r)=>{for(var o=r>1?void 0:r?Tx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ax(t,i,o),o};const Mx=["All","Debug","Info","Warn","Critical","Fatal"];let ot=class extends y{constructor(){super(...arguments),this.wsConnected=!1,this._entries=[],this._levelFilter="All",this._autoScroll=!0,this._loading=!0,this._error="",this._lastSeq=0,this._ignoreScrollEvent=!1,this._onLogEntry=e=>{const t=e.detail;!t||typeof t.seq!="number"||t.seq<=this._lastSeq||this._levelFilter!=="All"&&t.level!==this._levelFilter||(this._lastSeq=t.seq,this._entries=[...this._entries,t].slice(-1e3))},this._onScroll=()=>{if(this._ignoreScrollEvent)return;const e=this._logView;if(!e)return;const t=e.scrollHeight-e.scrollTop-e.clientHeight<48;this._autoScroll=t}}connectedCallback(){super.connectedCallback(),document.addEventListener("flow-log-entry",this._onLogEntry),this._loadInitial()}disconnectedCallback(){document.removeEventListener("flow-log-entry",this._onLogEntry),super.disconnectedCallback()}updated(e){(e.has("_entries")||e.has("_loading")&&!this._loading)&&this._scrollToBottom(),e.has("wsConnected")&&this.wsConnected&&this._syncSinceLast()}async _loadInitial(){this._loading=!0,this._error="";const e=await dh({limit:300,level:this._levelFilter});if(this._loading=!1,!e.ok){this._error=e.error||"加载失败";return}this._applyEntries(e.items)}async _syncSinceLast(){const e=await dh({limit:500,level:this._levelFilter,since:this._lastSeq});!e.ok||e.items.length===0||this._applyEntries([...this._entries,...e.items].slice(-1e3))}_applyEntries(e){const t=[...e].sort((i,r)=>i.seq-r.seq);this._entries=t,t.length>0&&(this._lastSeq=t[t.length-1].seq)}_scrollToBottom(){!this._autoScroll||this._loading||this._entries.length===0||this.updateComplete.then(()=>{requestAnimationFrame(()=>{const e=this._logView;if(!e)return;this._ignoreScrollEvent=!0;const t=e.querySelector(".log-row:last-child");t?t.scrollIntoView({block:"end"}):e.scrollTop=e.scrollHeight,requestAnimationFrame(()=>{this._ignoreScrollEvent=!1})})})}_resumeScroll(){this._autoScroll=!0,this._scrollToBottom()}async _onFilterChange(e){this._levelFilter=e.target.value,this._entries=[],this._lastSeq=0,await this._loadInitial()}_clearDisplay(){this._entries=[]}render(){return h`
      <div class="page">
        <div class="wrapper">
          <ha-card>
            <div class="card-body">
              <div class="toolbar">
                <div class="toolbar-left">
                  <h2 class="card-title">运行日志</h2>
                  <span class="status ${this.wsConnected?"live":"offline"}">
                    ${this.wsConnected?"实时":"离线"}
                  </span>
                  ${this._autoScroll?m:h`
                        <button type="button" class="resume" @click=${this._resumeScroll}>
                          回到底部
                        </button>
                      `}
                </div>
                <div class="toolbar-right">
                  <label class="filter">
                    <span>级别</span>
                    <select .value=${this._levelFilter} @change=${this._onFilterChange}>
                      ${Mx.map(e=>h`<option value=${e}>${e}</option>`)}
                    </select>
                  </label>
                  <button type="button" class="ghost" @click=${this._clearDisplay}>清空显示</button>
                  <button type="button" class="ghost" @click=${()=>void this._loadInitial()}>
                    刷新
                  </button>
                </div>
              </div>
              <p class="help">
                与桌面「终端显示」同步；WebSocket 连接时自动追加新日志。向上滚动可暂停自动滚底。
              </p>
              ${this._error?h`<p class="error">${this._error}</p>`:m}
              <div class="log-shell">
                <div class="log-view" @scroll=${this._onScroll}>
                  ${this._loading?h`<p class="empty">加载中…</p>`:this._entries.length===0?h`<p class="empty">暂无日志</p>`:this._entries.map(e=>h`
                            <div class="log-row ${wx(e.level)}">
                              <span class="ts">${e.timestamp}</span>
                              <span class="lvl">${e.level}</span>
                              <span class="msg">${e.message}</span>
                            </div>
                          `)}
                </div>
              </div>
            </div>
          </ha-card>
          <flow-log-history-card></flow-log-history-card>
        </div>
      </div>
    `}};ot.styles=w`
    :host {
      display: block;
      flex: 1;
      min-height: 0;
      overflow: hidden;
      --logs-panel-max-height: min(680px, calc(100dvh - var(--header-height, 56px) - 88px));
    }
    .page {
      height: 100%;
      min-height: 0;
      display: flex;
      flex-direction: column;
      overflow-y: auto;
      overflow-x: hidden;
      background: var(--primary-background-color, #e8edf1);
      color: var(--primary-text-color);
      font-family: var(--ha-font-family, system-ui, sans-serif);
    }
    .wrapper {
      flex: 0 0 auto;
      width: 100%;
      max-width: 1400px;
      max-height: var(--logs-panel-max-height);
      height: var(--logs-panel-max-height);
      margin: 0 auto;
      padding: 16px 24px 24px;
      box-sizing: border-box;
      display: flex;
      flex-direction: row;
      align-items: stretch;
      gap: 16px;
      overflow: hidden;
    }
    .wrapper > ha-card {
      flex: 2 1 0;
      min-width: 0;
      min-height: 0;
      display: flex;
      flex-direction: column;
    }
    .wrapper > flow-log-history-card {
      flex: 1 1 0;
      min-width: 0;
      min-height: 0;
      display: flex;
      flex-direction: column;
    }
    @media (max-width: 870px) {
      :host {
        flex: none;
        height: auto;
        min-height: 0;
        overflow: visible;
        --logs-panel-max-height: none;
      }
      .page {
        height: auto;
        min-height: 0;
        overflow: visible;
      }
      .wrapper {
        flex-direction: column;
        height: auto;
        max-height: none;
        overflow: visible;
        padding: 16px 16px 24px;
      }
      .wrapper > ha-card {
        flex: none;
        width: 100%;
        height: min(420px, calc(100dvh - var(--header-height, 56px) - 120px));
        min-height: 280px;
        max-height: none;
      }
      .wrapper > flow-log-history-card {
        flex: none;
        width: 100%;
        height: min(320px, calc(100dvh - var(--header-height, 56px) - 120px));
        min-height: 240px;
        max-height: none;
      }
    }
    ha-card {
      flex: 1;
      min-height: 0;
      max-height: 100%;
      overflow: hidden;
      display: flex;
      flex-direction: column;
    }
    .card-body {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      padding: 16px 18px;
      overflow: hidden;
    }
    .card-title {
      margin: 0;
      font-size: 16px;
      font-weight: 600;
    }
    .help {
      margin: 8px 0 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
      font-size: 13px;
      line-height: 1.45;
      flex-shrink: 0;
    }
    .toolbar {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 4px;
      flex-shrink: 0;
    }
    .toolbar-left,
    .toolbar-right {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 10px;
    }
    .status {
      font-size: 12px;
      padding: 2px 10px;
      border-radius: 999px;
      font-weight: 500;
      line-height: 1.6;
    }
    .status.live {
      background: rgba(46, 125, 50, 0.12);
      color: #2e7d32;
    }
    .status.offline {
      background: rgba(0, 0, 0, 0.06);
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.54));
    }
    .resume {
      padding: 2px 10px;
      border: 1px solid var(--primary-color, #03a9f4);
      border-radius: 999px;
      background: rgba(3, 169, 244, 0.08);
      color: var(--primary-color, #0288d1);
      font-size: 12px;
      cursor: pointer;
    }
    .filter {
      display: flex;
      align-items: center;
      gap: 6px;
      font-size: 13px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
    }
    select,
    button.ghost {
      padding: 6px 12px;
      border-radius: 8px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      background: var(--card-background-color, #fff);
      font-size: 13px;
      color: var(--primary-text-color, rgba(0, 0, 0, 0.87));
      cursor: pointer;
    }
    .error {
      color: var(--error-color, #c62828);
      font-size: 13px;
      margin: 0 0 8px;
    }
    .log-shell {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      margin-top: 4px;
      padding: 1px;
      border-radius: 12px;
      overflow: hidden;
      background: linear-gradient(
        180deg,
        rgba(0, 0, 0, 0.06) 0%,
        rgba(0, 0, 0, 0.03) 100%
      );
    }
    .log-view {
      flex: 1;
      min-height: 0;
      overflow: auto;
      padding: 12px 14px;
      border-radius: 11px;
      background: var(--log-surface, #f7f9fb);
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.08));
      box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.03);
      color: var(--primary-text-color, rgba(0, 0, 0, 0.87));
      font-family: ui-monospace, "Cascadia Code", "Consolas", monospace;
      font-size: 12px;
      line-height: 1.55;
    }
    .log-row {
      display: grid;
      grid-template-columns: 92px 76px minmax(0, 1fr);
      gap: 12px;
      align-items: start;
      padding: 6px 8px;
      border-radius: 6px;
      transition: background 0.12s ease;
    }
    .log-row:hover {
      background: rgba(0, 0, 0, 0.03);
    }
    .log-row + .log-row {
      margin-top: 2px;
    }
    .ts {
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
      white-space: nowrap;
      font-variant-numeric: tabular-nums;
    }
    .lvl {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 52px;
      padding: 1px 8px;
      border-radius: 6px;
      font-size: 11px;
      font-weight: 600;
      line-height: 1.5;
      white-space: nowrap;
    }
    .msg {
      word-break: break-word;
      white-space: pre-wrap;
      color: rgba(0, 0, 0, 0.78);
    }
    .level-debug .lvl {
      background: rgba(3, 169, 244, 0.12);
      color: #0277bd;
    }
    .level-info .lvl {
      background: rgba(2, 136, 209, 0.12);
      color: #01579b;
    }
    .level-warn .lvl {
      background: rgba(255, 152, 0, 0.14);
      color: #e65100;
    }
    .level-critical .lvl {
      background: rgba(211, 47, 47, 0.12);
      color: #b71c1c;
    }
    .empty {
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
      margin: 0;
      padding: 24px 8px;
      text-align: center;
    }
    @media (max-width: 720px) {
      .log-row {
        grid-template-columns: 1fr;
        gap: 4px;
      }
      .ts,
      .lvl {
        display: inline-flex;
      }
      .ts::after {
        content: " ·";
      }
    }
  `;Wt([u({type:Boolean})],ot.prototype,"wsConnected",2);Wt([g()],ot.prototype,"_entries",2);Wt([g()],ot.prototype,"_levelFilter",2);Wt([g()],ot.prototype,"_autoScroll",2);Wt([g()],ot.prototype,"_loading",2);Wt([g()],ot.prototype,"_error",2);Wt([g()],ot.prototype,"_lastSeq",2);Wt([mi(".log-view")],ot.prototype,"_logView",2);ot=Wt([x("flow-logs-panel")],ot);const Ox=["video","audio","dmx","image","model","document","childflow","unknown"],Lx={video:"视频",audio:"音频",dmx:"DMX",image:"图片",model:"3D 模型",document:"文档",childflow:"子流程",unknown:"其他"},Dx=new Set(["mp4","mov","mkv","avi","wmv","flv","webm"]),Ix=new Set(["wav","mp3","flac","aac","ogg","m4a"]),Hx=new Set(["dmx"]),zx=new Set(["jpg","jpeg","png","bmp","gif","webp","tiff"]),Vx=new Set(["obj","fbx","stl","gltf","glb"]),Nx=new Set(["txt","json","xml","cfg","log","md","csv","ini"]),Rx=new Set(["childflow"]);function Fx(e){const t=e.lastIndexOf("."),i=t>=0?e.slice(t+1).toLowerCase():"";return Dx.has(i)?"video":Ix.has(i)?"audio":Hx.has(i)?"dmx":zx.has(i)?"image":Vx.has(i)?"model":Rx.has(i)?"childflow":Nx.has(i)?"document":"unknown"}function Bx(e){const t=new Map;for(const i of e){const r=Fx(i.name),o=t.get(r);o?o.push(i):t.set(r,[i])}return Ox.flatMap(i=>{const r=t.get(i);return r!=null&&r.length?[{category:i,label:Lx[i],items:r}]:[]})}async function Ux(){try{const t=await(await fetch("/api/media/files")).json();return t.ok===!1?{ok:!1,items:[],error:t.error||"加载失败"}:{ok:!0,items:Array.isArray(t.items)?t.items:[]}}catch{return{ok:!1,items:[],error:"无法连接 Flow 媒体服务"}}}async function jx(e){try{const t=new URLSearchParams({file:e}),i=await fetch(`/api/media/download?${t.toString()}`);if(!i.ok)return!1;const r=await i.blob(),o=URL.createObjectURL(r),n=document.createElement("a");return n.href=o,n.download=e,n.click(),URL.revokeObjectURL(o),!0}catch{return!1}}function qx(e){return!Number.isFinite(e)||e<=0?"0 B":e<1024?`${e} B`:e<1024*1024?`${(e/1024).toFixed(1)} KB`:`${(e/(1024*1024)).toFixed(1)} MB`}function Gx(e){if(!e)return"—";const t=new Date(e);return Number.isNaN(t.getTime())?e:t.toLocaleString("zh-CN",{year:"numeric",month:"2-digit",day:"2-digit",hour:"2-digit",minute:"2-digit"})}var Xx=Object.defineProperty,Wx=Object.getOwnPropertyDescriptor,Yt=(e,t,i,r)=>{for(var o=r>1?void 0:r?Wx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Xx(t,i,o),o};let nt=class extends y{constructor(){super(...arguments),this._files=[],this._loading=!0,this._error="",this._downloading="",this._mediaFiles=[],this._mediaStatus="",this._mediaUploading=!1,this._mediaProgress={show:!1,percent:0,label:""},this._mediaDragOver=!1}connectedCallback(){super.connectedCallback(),this._loadLibrary()}async _loadLibrary(){this._loading=!0,this._error="";const e=await Ux();if(this._loading=!1,!e.ok){this._error=e.error||"加载失败",this._files=[];return}this._files=e.items}_setMediaFiles(e){const t=Array.from(e).map(i=>({file:i,status:"ready"}));this._mediaFiles=t}_onMediaInput(e){var i;const t=e.target;(i=t.files)!=null&&i.length&&this._setMediaFiles(t.files),t.value=""}_onMediaDrop(e){var t,i;e.preventDefault(),this._mediaDragOver=!1,(i=(t=e.dataTransfer)==null?void 0:t.files)!=null&&i.length&&this._setMediaFiles(e.dataTransfer.files)}_removeMedia(e){this._mediaFiles=this._mediaFiles.filter((t,i)=>i!==e)}async _uploadMedia(){const e=this._mediaFiles.map(i=>i.file);if(!e.length){this._mediaStatus="请选择媒体文件";return}this._mediaUploading=!0,this._mediaProgress={show:!0,percent:0,label:""};const t=[];try{for(let o=0;o<e.length;o++){const n=e[o],s=`${o+1}/${e.length} ${n.name}`;this._mediaProgress={...this._mediaProgress,label:s},this._mediaStatus=`正在上传：${s}`;const a=`/api/upload/media?filename=${encodeURIComponent(n.name)}`;try{await Eu(a,n,l=>{const c=o/e.length*100;this._mediaProgress={show:!0,label:s,percent:Math.min(100,Math.round(c+l/e.length))}}),t.push({name:n.name,ok:!0}),this._mediaFiles=this._mediaFiles.map((l,c)=>c===o?{...l,status:"success"}:l)}catch(l){const c=l instanceof Error?l.message:String(l);t.push({name:n.name,ok:!1,error:c}),this._mediaFiles=this._mediaFiles.map((d,p)=>p===o?{...d,status:"fail"}:d)}}this._mediaProgress={...this._mediaProgress,percent:100};const i=t.filter(o=>o.ok).length,r=t.length-i;this._mediaStatus=`完成：成功 ${i}，失败 ${r}`+(r?`；失败：${t.filter(o=>!o.ok).map(o=>`${o.name}(${o.error})`).join("；")}`:""),i>0&&await this._loadLibrary()}catch(i){this._mediaStatus=`上传异常：${i}`}finally{this._mediaUploading=!1}}async _download(e){this._downloading=e.name,this._error="";const t=await jx(e.name);this._downloading="",t||(this._error=`下载失败：${e.name}`)}_renderUploadIcon(){return h`
      <div class="upload-icon" aria-hidden="true">
        <svg viewBox="0 0 24 24" width="32" height="32" fill="currentColor">
          <path
            d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20M12,12L16,16H13.5V19H10.5V16H8L12,12Z"
          />
        </svg>
      </div>
    `}render(){return h`
      <div class="page">
        <div class="layout">
          <ha-card class="library-card">
            <div class="card-body">
              <div class="toolbar">
                <div>
                  <h2 class="card-title">媒体库</h2>
                  <p class="help">服务器上已保存的媒体文件，可下载到本地。</p>
                </div>
                <button type="button" class="ghost" ?disabled=${this._loading} @click=${()=>void this._loadLibrary()}>
                  刷新列表
                </button>
              </div>

              ${this._error?h`<p class="error">${this._error}</p>`:m}

              ${this._loading?h`<p class="empty">加载中…</p>`:this._files.length===0?h`<p class="empty">暂无媒体文件</p>`:h`
                      <div class="file-groups">
                        ${Bx(this._files).map(e=>h`
                            <ha-expandable-panel
                              class="file-group"
                              .header=${`${e.label} (${e.items.length})`}
                              expanded
                            >
                              <ul class="file-list">
                                ${e.items.map(t=>h`
                                    <li class="file-row">
                                      <div class="file-meta">
                                        <span class="file-name">${t.name}</span>
                                        <span class="file-sub">
                                          ${Gx(t.modified)} · ${qx(t.size)}
                                        </span>
                                      </div>
                                      <button
                                        type="button"
                                        class="download"
                                        ?disabled=${this._downloading===t.name}
                                        @click=${()=>void this._download(t)}
                                      >
                                        ${this._downloading===t.name?"下载中…":"下载"}
                                      </button>
                                    </li>
                                  `)}
                              </ul>
                            </ha-expandable-panel>
                          `)}
                      </div>
                    `}
            </div>
          </ha-card>

          <ha-card class="upload-card">
            <div class="card-body">
              <h2 class="card-title">上传媒体</h2>
              <p class="help">将文件拖拽到下方区域，或点击选择。支持多文件上传。</p>
              <div
                class="dropzone ${this._mediaDragOver?"dragover":""}"
                @dragover=${e=>{e.preventDefault(),this._mediaDragOver=!0}}
                @dragleave=${()=>{this._mediaDragOver=!1}}
                @drop=${this._onMediaDrop}
              >
                ${this._renderUploadIcon()}
                <p>将文件拖到此处，或 <em>点击选择</em></p>
                <input
                  class="file-input"
                  type="file"
                  multiple
                  accept=${lx}
                  ?disabled=${this._mediaUploading}
                  @change=${this._onMediaInput}
                />
              </div>
              <p class="upload-tip">支持常见音视频 / 图片，可一次选择多个文件</p>
              ${this._mediaFiles.length?h`
                    <ul class="pick-list">
                      ${this._mediaFiles.map((e,t)=>h`
                          <li class="pick-item ${e.status??""}">
                            <span>${e.file.name}</span>
                            <button
                              type="button"
                              class="link-btn"
                              ?disabled=${this._mediaUploading}
                              @click=${()=>this._removeMedia(t)}
                            >
                              移除
                            </button>
                          </li>
                        `)}
                    </ul>
                  `:m}
              <div class="actions">
                <button
                  type="button"
                  class="btn primary"
                  ?disabled=${this._mediaUploading}
                  @click=${()=>void this._uploadMedia()}
                >
                  ${this._mediaUploading?"上传中…":"上传到服务器"}
                </button>
                <span class="status">${this._mediaStatus}</span>
              </div>
              ${this._mediaProgress.show?h`
                    <div class="progress-wrap">
                      <div class="progress-bar">
                        <div class="progress-fill" style=${`width:${this._mediaProgress.percent}%`}></div>
                      </div>
                      <span class="progress-label">
                        ${this._mediaProgress.label||`${this._mediaProgress.percent}%`}
                      </span>
                    </div>
                  `:m}
            </div>
          </ha-card>
        </div>
      </div>
    `}};nt.styles=w`
    :host {
      display: block;
      height: 100%;
      min-height: 0;
    }
    .page {
      height: 100%;
      min-height: 0;
      padding: 16px 20px 24px;
      box-sizing: border-box;
    }
    .layout {
      display: grid;
      grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
      align-items: start;
      gap: 16px;
      height: 100%;
      max-height: min(680px, calc(100dvh - var(--header-height, 56px) - 88px));
      min-height: 0;
    }
    @media (max-width: 870px) {
      :host {
        height: auto;
        flex: none;
        overflow: visible;
      }
      .page {
        height: auto;
        min-height: 0;
        overflow: visible;
        padding: 16px 16px 24px;
      }
      .layout {
        grid-template-columns: 1fr;
        height: auto;
        max-height: none;
        align-items: stretch;
      }
      .library-card {
        height: min(420px, calc(100dvh - var(--header-height, 56px) - 120px));
        min-height: 280px;
        max-height: none;
      }
      .upload-card {
        height: auto;
      }
    }
    ha-card {
      min-height: 0;
      overflow: hidden;
      display: flex;
      flex-direction: column;
    }
    .library-card {
      height: 100%;
      max-height: inherit;
    }
    .upload-card {
      height: auto;
      align-self: start;
    }
    .upload-card .card-body {
      flex: none;
      overflow: visible;
    }
    .card-body {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      padding: 16px 18px;
      overflow: hidden;
    }
    .card-title {
      margin: 0 0 4px;
      font-size: 16px;
      font-weight: 600;
    }
    .help {
      margin: 0 0 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
      font-size: 13px;
      line-height: 1.45;
    }
    .btn.primary,
    button.ghost,
    button.download {
      padding: 8px 14px;
      border-radius: 8px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      background: var(--card-background-color, #fff);
      font-size: 13px;
      cursor: pointer;
      color: var(--primary-text-color, rgba(0, 0, 0, 0.87));
    }
    .btn.primary {
      border-color: rgba(3, 169, 244, 0.35);
      color: var(--primary-color, #0288d1);
      background: rgba(3, 169, 244, 0.06);
    }
    button.download {
      min-width: 72px;
      border-color: rgba(3, 169, 244, 0.35);
      color: var(--primary-color, #0288d1);
      background: rgba(3, 169, 244, 0.06);
    }
    button:disabled {
      opacity: 0.6;
      cursor: default;
    }
    .error {
      color: var(--error-color, #c62828);
      font-size: 13px;
      margin: 0 0 8px;
    }
    .empty {
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
      margin: 12px 0 0;
      font-size: 13px;
    }
    .dropzone {
      position: relative;
      border: 2px dashed var(--divider-color, rgba(0, 0, 0, 0.18));
      border-radius: 12px;
      padding: 16px 12px;
      text-align: center;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.55));
      background: var(--secondary-background-color, #f7f9fb);
      transition: border-color 0.15s, background 0.15s;
    }
    .dropzone.dragover {
      border-color: var(--primary-color, #0288d1);
      background: rgba(3, 169, 244, 0.06);
    }
    .dropzone p {
      margin: 8px 0 0;
      font-size: 14px;
    }
    .file-input {
      position: absolute;
      inset: 0;
      opacity: 0;
      cursor: pointer;
    }
    .upload-icon {
      color: var(--primary-color, #0288d1);
      opacity: 0.85;
    }
    .upload-tip {
      margin: 8px 0 0;
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
    }
    .pick-list {
      list-style: none;
      margin: 12px 0 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      gap: 6px;
      max-height: 120px;
      overflow: auto;
    }
    .pick-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 8px;
      padding: 6px 10px;
      border-radius: 8px;
      background: var(--secondary-background-color, #f7f9fb);
      font-size: 13px;
    }
    .pick-item.success {
      color: #2e7d32;
    }
    .pick-item.fail {
      color: #c62828;
    }
    .link-btn {
      border: none;
      background: none;
      color: var(--primary-color, #0288d1);
      cursor: pointer;
      font-size: 12px;
      padding: 0;
    }
    .actions {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 12px;
      margin-top: 12px;
    }
    .status {
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.55));
    }
    .progress-wrap {
      margin-top: 10px;
    }
    .progress-bar {
      height: 6px;
      border-radius: 999px;
      background: rgba(0, 0, 0, 0.08);
      overflow: hidden;
    }
    .progress-fill {
      height: 100%;
      background: var(--primary-color, #0288d1);
      transition: width 0.2s;
    }
    .progress-label {
      display: block;
      margin-top: 4px;
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.5));
    }
    .toolbar {
      display: flex;
      flex-wrap: wrap;
      align-items: flex-start;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 8px;
      flex-shrink: 0;
    }
    .file-groups {
      flex: 1;
      min-height: 0;
      overflow: auto;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .file-group {
      flex-shrink: 0;
    }
    .file-list {
      list-style: none;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .file-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 10px 12px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.08));
      border-radius: 10px;
      background: var(--secondary-background-color, #f7f9fb);
    }
    .file-meta {
      min-width: 0;
      display: flex;
      flex-direction: column;
      gap: 2px;
    }
    .file-name {
      font-family: ui-monospace, "Cascadia Code", "Consolas", monospace;
      font-size: 13px;
      word-break: break-all;
    }
    .file-sub {
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.54));
    }
  `;Yt([g()],nt.prototype,"_files",2);Yt([g()],nt.prototype,"_loading",2);Yt([g()],nt.prototype,"_error",2);Yt([g()],nt.prototype,"_downloading",2);Yt([g()],nt.prototype,"_mediaFiles",2);Yt([g()],nt.prototype,"_mediaStatus",2);Yt([g()],nt.prototype,"_mediaUploading",2);Yt([g()],nt.prototype,"_mediaProgress",2);nt=Yt([x("flow-media-panel")],nt);var Yx=Object.defineProperty,Zx=Object.getOwnPropertyDescriptor,_r=(e,t,i,r)=>{for(var o=r>1?void 0:r?Zx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Yx(t,i,o),o};let qt=class extends y{constructor(){super(...arguments),this.open=!1,this.title="",this.text="",this.confirmText="删除",this.cancelText="取消",this.destructive=!1,this._backdropDismiss=mr(()=>this._cancel())}_cancel(){this.dispatchEvent(new CustomEvent("dialog-result",{bubbles:!0,composed:!0,detail:{confirmed:!1}}))}_confirm(){this.dispatchEvent(new CustomEvent("dialog-result",{bubbles:!0,composed:!0,detail:{confirmed:!0}}))}render(){return this.open?h`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div
          class="dialog"
          role="alertdialog"
          aria-labelledby="confirm-title"
          aria-describedby="confirm-text"
        >
          <h2 id="confirm-title" class="title">${this.title}</h2>
          ${this.text?h`<p id="confirm-text" class="text">${this.text}</p>`:m}
          <div class="footer">
            <button type="button" class="btn cancel" @click=${this._cancel}>
              ${this.cancelText}
            </button>
            <button
              type="button"
              class="btn confirm ${this.destructive?"destructive":""}"
              @click=${this._confirm}
            >
              ${this.confirmText}
            </button>
          </div>
        </div>
      </div>
    `:m}};qt.styles=w`
    .backdrop {
      position: fixed;
      inset: 0;
      z-index: 300;
      background: rgba(0, 0, 0, 0.4);
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 24px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(360px, 100%);
      background: var(--card-background-color, #fff);
      border-radius: 28px;
      padding: 24px 24px 16px;
      box-shadow:
        0 11px 15px -7px rgba(0, 0, 0, 0.2),
        0 24px 38px 3px rgba(0, 0, 0, 0.14),
        0 9px 46px 8px rgba(0, 0, 0, 0.12);
    }
    .title {
      margin: 0 0 12px;
      font-size: 20px;
      font-weight: 500;
      line-height: 1.3;
      color: var(--primary-text-color);
    }
    .text {
      margin: 0 0 20px;
      font-size: 14px;
      line-height: 1.5;
      color: var(--secondary-text-color);
    }
    .footer {
      display: flex;
      justify-content: flex-end;
      align-items: center;
      gap: 8px;
      margin-top: 8px;
    }
    .btn {
      border: none;
      background: transparent;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
      font-weight: 500;
      padding: 10px 16px;
      border-radius: 20px;
      min-width: 64px;
    }
    .btn.cancel {
      color: var(--primary-color, #03a9f4);
    }
    .btn.cancel:hover {
      background: rgba(3, 169, 244, 0.08);
    }
    .btn.confirm {
      color: var(--primary-color, #03a9f4);
    }
    .btn.confirm.destructive {
      background: #f8b4b4;
      color: #5c1a1a;
    }
    .btn.confirm.destructive:hover {
      background: #f5a3a3;
    }
  `;_r([u({type:Boolean})],qt.prototype,"open",2);_r([u()],qt.prototype,"title",2);_r([u()],qt.prototype,"text",2);_r([u()],qt.prototype,"confirmText",2);_r([u()],qt.prototype,"cancelText",2);_r([u({type:Boolean})],qt.prototype,"destructive",2);qt=_r([x("flow-confirm-dialog")],qt);var Kx=Object.defineProperty,Qx=Object.getOwnPropertyDescriptor,$c=(e,t,i,r)=>{for(var o=r>1?void 0:r?Qx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Kx(t,i,o),o};let dn=class extends y{constructor(){super(...arguments),this.open=!1,this._backdropDismiss=mr(()=>this._cancel())}_cancel(){this.dispatchEvent(new CustomEvent("dialog-result",{bubbles:!0,composed:!0,detail:{confirmed:!1}}))}_confirm(){this.dispatchEvent(new CustomEvent("dialog-result",{bubbles:!0,composed:!0,detail:{confirmed:!0}}))}render(){return this.open?h`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog">
          ${this.cardConfig?h`
                <div class="preview">
                  <hui-card-container
                    .config=${this.cardConfig}
                    .editMode=${!1}
                  ></hui-card-container>
                </div>
              `:m}
          <div class="footer">
            <button type="button" class="btn cancel" @click=${this._cancel}>取消</button>
            <button type="button" class="btn confirm destructive" @click=${this._confirm}>
              删除
            </button>
          </div>
        </div>
      </div>
    `:m}};dn.styles=w`
    .backdrop {
      position: fixed;
      inset: 0;
      z-index: 300;
      background: rgba(0, 0, 0, 0.4);
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 24px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(420px, 100%);
      background: var(--card-background-color, #fff);
      border-radius: 28px;
      padding: 16px 16px 12px;
      box-shadow:
        0 11px 15px -7px rgba(0, 0, 0, 0.2),
        0 24px 38px 3px rgba(0, 0, 0, 0.14),
        0 9px 46px 8px rgba(0, 0, 0, 0.12);
    }
    .preview {
      position: relative;
      margin: 4px auto 12px;
      max-width: 500px;
      pointer-events: none;
    }
  /* hui-dialog-delete-card preview */
    .preview hui-card-container {
      display: block;
      width: 100%;
    }
    .footer {
      display: flex;
      justify-content: flex-end;
      align-items: center;
      gap: 8px;
    }
    .btn {
      border: none;
      background: transparent;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
      font-weight: 500;
      padding: 10px 16px;
      border-radius: 20px;
      min-width: 64px;
    }
    .btn.cancel {
      color: var(--primary-color, #03a9f4);
    }
    .btn.cancel:hover {
      background: rgba(3, 169, 244, 0.08);
    }
    .btn.confirm.destructive {
      background: #f8b4b4;
      color: #5c1a1a;
    }
    .btn.confirm.destructive:hover {
      background: #f5a3a3;
    }
  `;$c([u({type:Boolean})],dn.prototype,"open",2);$c([u({attribute:!1})],dn.prototype,"cardConfig",2);dn=$c([x("flow-delete-card-dialog")],dn);var Jx=Object.defineProperty,ew=Object.getOwnPropertyDescriptor,Ei=(e,t,i,r)=>{for(var o=r>1?void 0:r?ew(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Jx(t,i,o),o};let kt=class extends y{constructor(){super(...arguments),this.open=!1,this.title="",this._error="",this._statesRevision=0,this._backdropDismiss=mr(()=>this._close()),this._loadToken=0,this._entityUnsubs=[]}disconnectedCallback(){this._clearEntitySubscriptions(),super.disconnectedCallback()}updated(e){if(!this.open){e.has("open")&&(this._clearEntitySubscriptions(),this._card=void 0,this._error="");return}if(e.has("open")||e.has("cardConfig")){this._mountCard();return}e.has("flow")&&this._card&&(this._card.flow=this.flow,this._syncEntitySubscriptions(),this._refreshCard())}_refreshCard(){var t;const e=this._card;(t=e==null?void 0:e.requestUpdate)==null||t.call(e),this.requestUpdate()}_clearEntitySubscriptions(){this._entityUnsubs.forEach(e=>e()),this._entityUnsubs=[]}_syncEntitySubscriptions(){this._clearEntitySubscriptions();const e=this.flow;if(!e||!this._card)return;const t=Gp(this.cardConfig);if(t.length===0){this._entityUnsubs.push(e.subscribeStates(()=>{this._statesRevision=e.statesRevision,this._refreshCard()}));return}t.forEach(i=>{this._entityUnsubs.push(e.subscribeEntity(i,()=>{this._statesRevision=e.statesRevision,this._refreshCard()}))})}async _mountCard(){const e=this.cardConfig;if(!e){this._clearEntitySubscriptions(),this._card=void 0,this._error="未配置弹出卡片";return}const t=++this._loadToken;this._error="";try{const i=await Ks(e);if(t!==this._loadToken)return;i.flow=this.flow,this._card=i,this._syncEntitySubscriptions()}catch(i){if(t!==this._loadToken)return;this._clearEntitySubscriptions(),this._card=void 0,this._error=i instanceof Error?i.message:String(i)}}_close(){this.dispatchEvent(new CustomEvent("dialog-closed",{bubbles:!0,composed:!0}))}render(){var t;if(!this.open)return m;this._statesRevision;const e=this.title.trim()||String(((t=this.cardConfig)==null?void 0:t.name)??"控制");return h`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog" role="dialog" aria-modal="true" aria-label=${e}>
          <header class="header">
            <h2 class="title">${e}</h2>
            <button type="button" class="close" aria-label="关闭" @click=${this._close}>×</button>
          </header>
          <div class="body">
            ${this._error?h`<p class="error">${this._error}</p>`:this._card?this._card:h`<p class="loading">加载中…</p>`}
          </div>
        </div>
      </div>
    `}};kt.styles=w`
    .backdrop {
      position: fixed;
      inset: 0;
      z-index: 320;
      background: rgba(0, 0, 0, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 24px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(560px, 100%);
      max-height: min(86vh, 720px);
      overflow: auto;
      background: var(--primary-background-color, #f5f5f5);
      border-radius: 20px;
      box-shadow:
        0 11px 15px -7px rgba(0, 0, 0, 0.2),
        0 24px 38px 3px rgba(0, 0, 0, 0.14),
        0 9px 46px 8px rgba(0, 0, 0, 0.12);
    }
    .header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 16px 16px 8px;
      position: sticky;
      top: 0;
      background: var(--primary-background-color, #f5f5f5);
      z-index: 1;
    }
    .title {
      margin: 0;
      font-size: 18px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .close {
      width: 32px;
      height: 32px;
      border: none;
      border-radius: 50%;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--primary-text-color);
      font-size: 22px;
      line-height: 1;
      cursor: pointer;
    }
    .body {
      padding: 8px 16px 16px;
    }
    .body > * {
      display: block;
      width: 100%;
    }
    .loading,
    .error {
      margin: 12px 0;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .error {
      color: var(--error-color, #db4437);
    }
  `;Ei([u({type:Boolean})],kt.prototype,"open",2);Ei([u({attribute:!1})],kt.prototype,"flow",2);Ei([u({attribute:!1})],kt.prototype,"cardConfig",2);Ei([u()],kt.prototype,"title",2);Ei([g()],kt.prototype,"_card",2);Ei([g()],kt.prototype,"_error",2);Ei([g()],kt.prototype,"_statesRevision",2);kt=Ei([x("flow-badge-card-dialog")],kt);var tw=Object.defineProperty,iw=Object.getOwnPropertyDescriptor,Tn=(e,t,i,r)=>{for(var o=r>1?void 0:r?iw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&tw(t,i,o),o};const rw="Flow Dashboard",hh="flow_sidebar_expanded";let lr=class extends y{constructor(){super(...arguments),this.activePanel="home",this.connected=!1,this.narrow=!1,this._expanded=!0}connectedCallback(){super.connectedCallback();try{const e=localStorage.getItem(hh);e!==null&&(this._expanded=e==="true")}catch{}}get _isExpanded(){return this.narrow||this._expanded}_toggleExpanded(){if(this.narrow){Y(this,"hass-toggle-menu",{open:!1});return}this._expanded=!this._expanded;try{localStorage.setItem(hh,String(this._expanded))}catch{}Y(this,"hass-dock-sidebar",{dock:this._expanded?"docked":"auto"})}_select(e){this.dispatchEvent(new CustomEvent("panel-selected",{bubbles:!0,composed:!0,detail:{panel:e}})),this.narrow&&Y(this,"hass-toggle-menu",{open:!1})}_renderNavItem(e){const t=this.activePanel===e.panel;return h`
      <button
        type="button"
        class="nav-item ${t?"selected":""}"
        title=${e.label}
        aria-current=${t?"page":"false"}
        @click=${()=>this._select(e.panel)}
      >
        <ha-svg-icon class="nav-icon" .path=${e.iconPath}></ha-svg-icon>
        <span class="nav-label">${e.label}</span>
        ${e.badge?h`<span class="nav-badge">${e.badge}</span>`:m}
      </button>
    `}render(){const e=this._isExpanded,t=e?$f:Ph,i=this.connected?void 0:"!";return h`
      <aside
        class="sidebar ${e?"expanded":"collapsed"}"
        aria-label="主导航"
      >
        <div class="menu-header">
          <button
            type="button"
            class="menu-toggle"
            aria-label=${this._expanded?"收起侧边栏":"展开侧边栏"}
            @click=${this._toggleExpanded}
          >
            <ha-svg-icon .path=${t}></ha-svg-icon>
          </button>
          <span class="menu-title">${rw}</span>
        </div>

        <div class="sidebar-body">
          <div class="panel-list top-list">
            ${this._renderNavItem({panel:"home",iconPath:vf,label:"Home"})}
            ${this._renderNavItem({panel:"actions",iconPath:xf,label:"动作库"})}
            ${this._renderNavItem({panel:"logs",iconPath:gf,label:"日志"})}
            ${this._renderNavItem({panel:"media",iconPath:kf,label:"媒体"})}
          </div>

          <div class="spacer"></div>

          <div class="panel-list bottom-list">
            ${this._renderNavItem({panel:"config",iconPath:bf,label:"设置",badge:i})}
          </div>
        </div>
      </aside>
    `}};lr.styles=w`
    :host {
      display: block;
      height: 100%;
      width: 100%;
    }

    .sidebar {
      --sidebar-item-width: 48px;
      --sidebar-expanded-item-width: 248px;
      display: flex;
      flex-direction: column;
      height: 100%;
      width: 100%;
      background: var(--sidebar-background-color, #ffffff);
      border-right: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      box-sizing: border-box;
      overflow: hidden;
    }

    .menu-header {
      display: flex;
      align-items: center;
      flex-shrink: 0;
      height: calc(var(--header-height, 56px) + var(--safe-area-inset-top, 0px));
      padding-top: var(--safe-area-inset-top, 0px);
      padding-left: calc(4px + var(--safe-area-inset-left, 0px));
      padding-right: 8px;
      border-bottom: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      overflow: hidden;
      white-space: nowrap;
      background: inherit;
    }

    .menu-toggle {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 48px;
      height: 48px;
      border: none;
      border-radius: 50%;
      background: transparent;
      color: var(--sidebar-icon-color, rgba(0, 0, 0, 0.54));
      cursor: pointer;
      flex-shrink: 0;
    }

    .menu-toggle:hover {
      background: rgba(0, 0, 0, 0.04);
    }

    .menu-toggle ha-svg-icon {
      width: 24px;
      height: 24px;
    }

    .menu-title {
      font-size: 20px;
      font-weight: 400;
      line-height: 1.2;
      color: var(--sidebar-text-color, rgba(0, 0, 0, 0.87));
      opacity: 0;
      max-width: 0;
      overflow: hidden;
      transition:
        max-width 0.2s ease,
        opacity 0.2s ease;
    }

    .sidebar.expanded .menu-title {
      opacity: 1;
      max-width: 220px;
      transition-delay: 0ms, 80ms;
    }

    .sidebar-body {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      padding-bottom: calc(8px + var(--safe-area-inset-bottom, 0px));
    }

    .panel-list {
      display: flex;
      flex-direction: column;
      padding: 8px 4px;
      gap: 4px;
      flex-shrink: 0;
    }

    .spacer {
      flex: 1;
      min-height: 8px;
    }

    .nav-item {
      position: relative;
      display: flex;
      align-items: center;
      width: var(--sidebar-item-width);
      min-height: 40px;
      margin: 0 4px;
      padding: 0 12px;
      border: none;
      border-radius: 8px;
      background: transparent;
      color: var(--sidebar-icon-color, rgba(0, 0, 0, 0.54));
      cursor: pointer;
      text-align: left;
      transition: width 0.2s ease;
    }

    .sidebar.expanded .nav-item {
      width: var(--sidebar-expanded-item-width);
    }

    .nav-item:hover {
      background: rgba(0, 0, 0, 0.04);
    }

    .nav-item.selected {
      color: var(--sidebar-selected-icon-color, var(--primary-color, #03a9f4));
    }

    .nav-item.selected::before {
      content: "";
      position: absolute;
      inset: 0;
      border-radius: inherit;
      background: var(--sidebar-selected-icon-color, var(--primary-color, #03a9f4));
      opacity: 0.12;
      pointer-events: none;
    }

    .nav-icon {
      width: 24px;
      height: 24px;
      flex-shrink: 0;
      position: relative;
      z-index: 1;
    }

    .nav-label {
      position: relative;
      z-index: 1;
      margin-left: 12px;
      font-size: 14px;
      font-weight: 500;
      line-height: 1.25;
      color: var(--sidebar-text-color, rgba(0, 0, 0, 0.87));
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
      opacity: 0;
      max-width: 0;
      transition:
        max-width 0.2s ease,
        opacity 0.2s ease;
    }

    .sidebar.expanded .nav-label {
      opacity: 1;
      max-width: 180px;
      transition-delay: 0ms, 80ms;
    }

    .nav-item.selected .nav-label {
      color: var(--sidebar-selected-icon-color, var(--primary-color, #03a9f4));
    }

    .nav-badge {
      margin-left: auto;
      min-width: 18px;
      height: 18px;
      padding: 0 5px;
      border-radius: 9px;
      background: var(--accent-color, #ff9800);
      color: #fff;
      font-size: 11px;
      font-weight: 500;
      line-height: 18px;
      text-align: center;
      position: relative;
      z-index: 1;
      flex-shrink: 0;
      opacity: 0;
      transform: scale(0.8);
      transition: opacity 0.2s ease, transform 0.2s ease;
    }

    .sidebar.expanded .nav-badge {
      opacity: 1;
      transform: scale(1);
    }

    .sidebar.collapsed .menu-header {
      justify-content: center;
      padding-left: var(--safe-area-inset-left, 0px);
      padding-right: 0;
    }

    .sidebar.collapsed .panel-list {
      align-items: center;
      padding-left: 0;
      padding-right: 0;
    }

    .sidebar.collapsed .nav-item {
      width: 48px;
      margin: 0;
      padding: 0;
      justify-content: center;
    }

    .sidebar.collapsed .nav-label {
      position: absolute;
      width: 1px;
      height: 1px;
      margin: 0;
      padding: 0;
      overflow: hidden;
      clip: rect(0, 0, 0, 0);
      white-space: nowrap;
      border: 0;
      opacity: 0;
    }

    .sidebar.collapsed .nav-badge {
      position: absolute;
      top: 2px;
      right: 6px;
      left: auto;
      margin-left: 0;
      min-width: 16px;
      height: 16px;
      line-height: 16px;
      font-size: 10px;
      padding: 0 4px;
      opacity: 1;
      transform: scale(1);
    }

    @media (prefers-reduced-motion: reduce) {
      .sidebar,
      .menu-title,
      .nav-item,
      .nav-label,
      .nav-badge {
        transition: none;
      }
    }
  `;Tn([u({attribute:!1})],lr.prototype,"activePanel",2);Tn([u({type:Boolean})],lr.prototype,"connected",2);Tn([u({type:Boolean,reflect:!0})],lr.prototype,"narrow",2);Tn([g()],lr.prototype,"_expanded",2);lr=Tn([x("ha-sidebar")],lr);var ow=Object.defineProperty,nw=Object.getOwnPropertyDescriptor,kc=(e,t,i,r)=>{for(var o=r>1?void 0:r?nw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ow(t,i,o),o};let hn=class extends y{constructor(){super(...arguments),this.type="",this.open=!1,this._touchStartX=0,this._onTouchStart=e=>{var t;!this._modal||!this.open||(this._touchStartX=((t=e.touches[0])==null?void 0:t.clientX)??0)},this._onTouchEnd=e=>{var i;if(!this._modal||!this.open)return;const t=((i=e.changedTouches[0])==null?void 0:i.clientX)??this._touchStartX;this._touchStartX-t>72&&this._close()}}get _modal(){return this.type==="modal"}_close(){this.open&&(this.open=!1,Y(this,"hass-drawer-closed"))}updated(){document.body.style.overflow=this._modal&&this.open?"hidden":""}disconnectedCallback(){document.body.style.overflow="",super.disconnectedCallback()}render(){return this._modal?h`
        <div class="layout modal">
          <div
            class="scrim ${this.open?"visible":""}"
            @click=${this._close}
          ></div>
          <aside
            class="modal-drawer ${this.open?"open":""}"
            @touchstart=${this._onTouchStart}
            @touchend=${this._onTouchEnd}
          >
            <slot name="sidebar"></slot>
          </aside>
          <div class="app-content">
            <slot></slot>
          </div>
        </div>
      `:h`
      <div class="layout">
        <aside class="sidebar-shell">
          <slot name="sidebar"></slot>
        </aside>
        <div class="app-content">
          <slot></slot>
        </div>
      </div>
    `}};hn.styles=w`
    :host {
      display: block;
      height: 100%;
    }
    .layout {
      display: flex;
      height: 100%;
      min-height: 0;
    }
    .sidebar-shell {
      flex: none;
      width: var(--ha-sidebar-width, 56px);
      height: 100%;
      box-sizing: border-box;
      overflow: hidden;
      z-index: 6;
      transition: width 180ms ease;
    }
    .app-content {
      flex: 1;
      min-width: 0;
      min-height: 0;
      height: 100%;
      display: flex;
      flex-direction: column;
      box-sizing: border-box;
    }
    .app-content ::slotted(*) {
      flex: 1;
      min-height: 0;
      height: 100%;
    }
    .sidebar-shell ::slotted(*),
    .modal-drawer ::slotted(*) {
      display: block;
      width: 100%;
      height: 100%;
    }
    .layout.modal .app-content {
      width: 100%;
    }
    .scrim {
      position: fixed;
      inset: 0;
      background: rgba(0, 0, 0, 0.32);
      opacity: 0;
      pointer-events: none;
      transition: opacity 180ms ease;
      z-index: 199;
    }
    .scrim.visible {
      opacity: 1;
      pointer-events: auto;
    }
    .modal-drawer {
      position: fixed;
      top: 0;
      bottom: 0;
      left: 0;
      width: calc(256px + var(--safe-area-inset-left, 0px));
      max-width: 86vw;
      background: var(--sidebar-background-color, #fff);
      box-shadow: 0 8px 10px -5px rgba(0, 0, 0, 0.2), 0 16px 24px 2px rgba(0, 0, 0, 0.14),
        0 6px 30px 5px rgba(0, 0, 0, 0.12);
      transform: translateX(-105%);
      transition: transform 200ms ease;
      z-index: 200;
      overflow: hidden;
    }
    .modal-drawer.open {
      transform: translateX(0);
    }
  `;kc([u({reflect:!0})],hn.prototype,"type",2);kc([u({type:Boolean,reflect:!0})],hn.prototype,"open",2);hn=kc([x("ha-drawer")],hn);var sw=Object.defineProperty,aw=Object.getOwnPropertyDescriptor,Cc=(e,t,i,r)=>{for(var o=r>1?void 0:r?aw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&sw(t,i,o),o};let pn=class extends y{constructor(){super(...arguments),this.narrow=!1,this.hasNotifications=!1}render(){return this.narrow?h`
      <ha-icon-button
        .path=${Ph}
        label="菜单"
        @click=${this._toggleMenu}
      ></ha-icon-button>
      ${this.hasNotifications?h`<span class="dot"></span>`:m}
    `:m}_toggleMenu(){Y(this,"hass-toggle-menu")}};pn.styles=w`
    :host {
      position: relative;
      display: inline-flex;
    }
    .dot {
      pointer-events: none;
      position: absolute;
      background-color: var(--accent-color);
      width: 12px;
      height: 12px;
      top: 9px;
      right: 7px;
      border-radius: 50%;
      border: 2px solid var(--app-header-background-color, #fff);
    }
  `;Cc([u({type:Boolean})],pn.prototype,"narrow",2);Cc([u({type:Boolean})],pn.prototype,"hasNotifications",2);pn=Cc([x("ha-menu-button")],pn);var lw=Object.defineProperty,cw=Object.getOwnPropertyDescriptor,F=(e,t,i,r)=>{for(var o=r>1?void 0:r?cw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&lw(t,i,o),o};const ph="flow_sidebar_expanded",dw="(max-width: 870px)";let z=class extends y{constructor(){super(...arguments),this._activeView=0,this._panel="home",this._editMode=!1,this._pickerOpen=!1,this._cardEditorOpen=!1,this._sectionEditorOpen=!1,this._viewEditorOpen=!1,this._viewEditorIsNew=!1,this._confirmOpen=!1,this._deleteCardOpen=!1,this._badgeCardOpen=!1,this._layoutEditEnabled=!1,this._layoutRevision=0,this._narrow=!1,this._drawerOpen=!1,this._sidebarExpanded=!0,this._connected=!1,this._applyingUndoRedo=!1,this._syncingHash=!1,this._undoRedo=new jv({currentConfig:()=>({viewIndex:this._activeView,config:structuredClone(this.lovelace)}),apply:e=>this._applyUndoRedo(e),onStackChange:()=>this.requestUpdate()}),this._onKeyDown=e=>{!this._editMode||this._panel!=="home"||!(e.ctrlKey||e.metaKey)||(e.key==="z"&&!e.shiftKey?(e.preventDefault(),this._undo()):(e.key==="y"||e.key==="z"&&e.shiftKey)&&(e.preventDefault(),this._redo()))},this._onShowConfirm=e=>{this._confirmParams=e.detail,this._confirmOpen=!0},this._onShowDeleteCard=e=>{const t=e.detail;this._deleteCardConfig=t.cardConfig,this._deleteCardOpen=!0},this._onShowBadgeCard=e=>{this._badgeCardParams=e.detail,this._badgeCardOpen=!0},this._onBadgeCardClosed=()=>{this._badgeCardOpen=!1,this._badgeCardParams=void 0,Yf()},this._onLayoutEditEnabled=e=>{var i;const t=!!((i=e.detail)!=null&&i.enabled);this._layoutEditEnabled=t,!t&&this._editMode&&this._exitEditMode()},this._onHashChange=()=>{this._syncingHash||this._syncViewFromHash()},this._onViewSelected=e=>{this._panel="home",this._selectView(e.detail.index)},this._onViewAdd=()=>{!this.lovelace||!this._editMode||(this._viewEditorIsNew=!0,this._viewEditorTarget=void 0,this._viewEditorOpen=!0)},this._onViewEdit=e=>{!this.lovelace||!this._editMode||(this._viewEditorIsNew=!1,this._viewEditorTarget=e.detail.index,this._viewEditorOpen=!0)},this._onViewDelete=e=>{this._deleteView(e.detail.index)},this._onLayoutWillChange=()=>{this._commitBeforeChange()},this._onLayoutChanged=()=>{this._notifyLayoutChanged()},this._onPanelSelected=e=>{this._editMode&&(this._editMode=!1,this._undoRedo.reset()),this._panel=e.detail.panel,this._narrow&&(this._drawerOpen=!1),this._panel==="home"&&this._writeViewHash(this._activeView,!0)},this._onToggleMenu=e=>{const t=e.detail;if(this._narrow){this._drawerOpen=(t==null?void 0:t.open)??!this._drawerOpen;return}this._sidebarExpanded=(t==null?void 0:t.open)??!this._sidebarExpanded;try{localStorage.setItem(ph,String(this._sidebarExpanded))}catch{}},this._onDockSidebar=e=>{var i;const t=(i=e.detail)==null?void 0:i.dock;this._sidebarExpanded=t==="docked"},this._onAddSection=()=>{const e=this._view();e&&(this._commitBeforeChange(),vv(e,kh(!0)),this._notifyLayoutChanged())},this._onAddCard=e=>{this._pickerSection=e.detail.section,this._pickerOpen=!0},this._onEditCard=e=>{this._cardEditorTarget=e.detail,this._cardEditorOpen=!0},this._onDuplicateCard=e=>{const t=this._view();if(!t)return;const i=t.sections[e.detail.sectionIndex];i&&(this._commitBeforeChange(),ni(t,e.detail.sectionIndex,wv(i,e.detail.cardIndex)),this._notifyLayoutChanged())},this._onCopyCard=e=>{const t=this._view();if(!t)return;const i=t.sections[e.detail.sectionIndex],r=i==null?void 0:i.cards[e.detail.cardIndex];r&&Nv(structuredClone(r))},this._onDeleteCard=e=>{this._deleteCard(e)},this._onEditSection=e=>{this._sectionEditorTarget=e.detail.sectionIndex,this._sectionEditorOpen=!0},this._onDuplicateSection=e=>{const t=this._view();t&&(this._commitBeforeChange(),kv(t,e.detail.sectionIndex),this._notifyLayoutChanged())},this._onDeleteSection=e=>{this._deleteSection(e)}}connectedCallback(){super.connectedCallback();try{const e=localStorage.getItem(ph);e!==null&&(this._sidebarExpanded=e==="true")}catch{}this._layoutEditEnabled=Sp(),document.addEventListener(ol,this._onLayoutEditEnabled),this._unsubMediaQuery=qv(dw,e=>{this._narrow=e,e||(this._drawerOpen=!1)}),document.addEventListener("flow-show-confirm",this._onShowConfirm),document.addEventListener("flow-show-delete-card",this._onShowDeleteCard),document.addEventListener("flow-show-badge-card",this._onShowBadgeCard),this.addEventListener("add-section",this._onAddSection),this.addEventListener("add-card",this._onAddCard),this.addEventListener("layout-will-change",this._onLayoutWillChange),this.addEventListener("layout-changed",this._onLayoutChanged),this.addEventListener("ll-edit-card",this._onEditCard),this.addEventListener("ll-duplicate-card",this._onDuplicateCard),this.addEventListener("ll-copy-card",this._onCopyCard),this.addEventListener("ll-delete-card",this._onDeleteCard),this.addEventListener("ll-edit-section",this._onEditSection),this.addEventListener("ll-duplicate-section",this._onDuplicateSection),this.addEventListener("ll-delete-section",this._onDeleteSection),this.addEventListener("hass-toggle-menu",this._onToggleMenu),this.addEventListener("hass-dock-sidebar",this._onDockSidebar),window.addEventListener("keydown",this._onKeyDown),window.addEventListener("hashchange",this._onHashChange),this._syncViewFromHash()}disconnectedCallback(){var e,t;(e=this._unsubMediaQuery)==null||e.call(this),(t=this._unsubConnection)==null||t.call(this),document.removeEventListener("flow-show-confirm",this._onShowConfirm),document.removeEventListener("flow-show-delete-card",this._onShowDeleteCard),document.removeEventListener("flow-show-badge-card",this._onShowBadgeCard),document.removeEventListener(ol,this._onLayoutEditEnabled),this.removeEventListener("add-section",this._onAddSection),this.removeEventListener("add-card",this._onAddCard),this.removeEventListener("layout-will-change",this._onLayoutWillChange),this.removeEventListener("layout-changed",this._onLayoutChanged),this.removeEventListener("ll-edit-card",this._onEditCard),this.removeEventListener("ll-duplicate-card",this._onDuplicateCard),this.removeEventListener("ll-copy-card",this._onCopyCard),this.removeEventListener("ll-delete-card",this._onDeleteCard),this.removeEventListener("ll-edit-section",this._onEditSection),this.removeEventListener("ll-duplicate-section",this._onDuplicateSection),this.removeEventListener("ll-delete-section",this._onDeleteSection),this.removeEventListener("hass-toggle-menu",this._onToggleMenu),this.removeEventListener("hass-dock-sidebar",this._onDockSidebar),window.removeEventListener("keydown",this._onKeyDown),window.removeEventListener("hashchange",this._onHashChange),super.disconnectedCallback()}updated(e){this.toggleAttribute("expanded",this._sidebarExpanded&&!this._narrow),this.toggleAttribute("modal",this._narrow),e.has("flow")&&this._bindConnection(),e.has("lovelace")&&this.lovelace&&(this._clampActiveView(),this._syncViewFromHash())}_view(){if(this.lovelace)return bv(this.lovelace,this._activeView)}_clampActiveView(){var e;if(!((e=this.lovelace)!=null&&e.views.length)){this._activeView=0;return}this._activeView>=this.lovelace.views.length&&(this._activeView=this.lovelace.views.length-1),this._activeView<0&&(this._activeView=0)}_hashViewPath(){const e=location.hash.replace(/^#\/?/,"").trim();if(!e)return;const t=decodeURIComponent(e.split(/[/?#]/)[0]??"");if(!(!t||t==="config"||t==="settings"||t==="actions"||t==="logs"||t==="media"))return t}_syncViewFromHash(){if(!this.lovelace)return;const e=this._hashViewPath();if(e===void 0){this._writeViewHash(this._activeView,!0);return}const t=yd(this.lovelace,e);t!==this._activeView&&(this._activeView=t,this._closeEditors())}_writeViewHash(e,t=!1){if(!this.lovelace)return;const i=this.lovelace.views[e];if(!i)return;const r=`#/${rl(i,e)}`;location.hash!==r&&(this._syncingHash=!0,t?history.replaceState(null,"",`${location.pathname}${location.search}${r}`):location.hash=r,queueMicrotask(()=>{this._syncingHash=!1}))}_selectView(e,t){if(!this.lovelace)return;const i=yd(this.lovelace,e);if(i===this._activeView){this._writeViewHash(i,(t==null?void 0:t.replaceHash)??!1);return}this._activeView=i,this._closeEditors(),this._writeViewHash(i,(t==null?void 0:t.replaceHash)??!1)}_closeEditors(){this._pickerOpen=!1,this._pickerSection=void 0,this._cardEditorOpen=!1,this._cardEditorTarget=void 0,this._sectionEditorOpen=!1,this._sectionEditorTarget=void 0,this._viewEditorOpen=!1,this._viewEditorTarget=void 0,this._viewEditorIsNew=!1}_reservedViewPaths(e){return this.lovelace?this.lovelace.views.map((t,i)=>i===e?void 0:t.path).filter(t=>!!t):[]}_saveViewEdit(e){if(!this.lovelace||!this._editMode)return;const t=e.detail.config;if(this._commitBeforeChange(),e.detail.isNew){const r=Fc({title:t.title,path:t.path,icon:t.icon});r.max_columns=t.max_columns??4,t.dense_section_placement&&(r.dense_section_placement=!0);const o=Pv(this.lovelace,r);this._viewEditorOpen=!1,this._viewEditorIsNew=!1,this._viewEditorTarget=void 0,this._notifyLayoutChanged(),this._selectView(o);return}const i=this._viewEditorTarget??this._activeView;if(Tv(this.lovelace,i,{title:t.title,path:t.path,icon:t.icon,max_columns:t.max_columns,dense_section_placement:t.dense_section_placement}),!t.dense_section_placement){const r=this.lovelace.views[i];r&&delete r.dense_section_placement}this._viewEditorOpen=!1,this._viewEditorTarget=void 0,this._notifyLayoutChanged(),this._writeViewHash(i,!0)}async _deleteView(e){if(!this.lovelace||!this._editMode||this.lovelace.views.length<=1)return;const t=this.lovelace.views[e];if(!t)return;const i=t.title||t.path||`视图 ${e+1}`;if(!await il({title:"删除视图",text:`确定删除「${i}」及其所有分区与卡片？`,confirmText:"删除",destructive:!0}))return;this._commitBeforeChange();const o=Av(this.lovelace,e);this._notifyLayoutChanged(),this._selectView(o,{replaceHash:!0})}_commitBeforeChange(){!this._editMode||!this.lovelace||this._applyingUndoRedo||(this._undoRedo.commit({viewIndex:this._activeView,config:structuredClone(this.lovelace)}),this.requestUpdate())}_notifyLayoutChanged(){this._layoutRevision+=1,this._persist()}_headerTitle(e){return this._panel==="config"?"设置":this._panel==="actions"?"动作库":this._panel==="logs"?"运行日志":this._panel==="media"?"媒体库":e}_applyUndoRedo(e){this._applyingUndoRedo=!0,this.lovelace=structuredClone(e.config),this._activeView=e.viewIndex,this._clampActiveView(),this._layoutRevision+=1,this._applyingUndoRedo=!1,this._writeViewHash(this._activeView,!0),this._persist()}_undo(){this._undoRedo.undo()}_redo(){this._undoRedo.redo()}async _deleteCard(e){const t=this._view();if(!t)return;const i=t.sections[e.detail.sectionIndex];if(!i)return;const r=i.cards[e.detail.cardIndex];!e.detail.silent&&!await gv(r)||(this._commitBeforeChange(),ni(t,e.detail.sectionIndex,$v(i,e.detail.cardIndex)),this._notifyLayoutChanged())}_onConfirmResult(e){this._confirmOpen=!1,this._confirmParams=void 0,fv(e.detail.confirmed)}_onDeleteCardResult(e){this._deleteCardOpen=!1,this._deleteCardConfig=void 0,mv(e.detail.confirmed)}async _deleteSection(e){var o;const t=this._view();if(!t)return;const i=t.sections[e.detail.sectionIndex];!i||(((o=i.cards)==null?void 0:o.length)??0)>0&&!await il({title:"删除部件",text:"此部件及其所有卡片都将被删除。",confirmText:"删除",destructive:!0})||(this._commitBeforeChange(),Cv(t,e.detail.sectionIndex),this._notifyLayoutChanged())}async _pickCard(e){var o;const t=this._view();if(!this._pickerSection||!t)return;const i=t.sections.indexOf(this._pickerSection);if(i<0)return;let r;if(e.config)r=structuredClone(e.config);else{const n=await ml(e.type);r=((o=n==null?void 0:n.getStubConfig)==null?void 0:o.call(n))??{type:e.type}}Fv(r.type),this._commitBeforeChange(),ni(t,i,_v(this._pickerSection,r)),this._pickerOpen=!1,this._pickerSection=void 0,this._notifyLayoutChanged()}async _saveCardEdit(e){const t=this._view(),i=this._cardEditorTarget;if(!t||!i)return;const r=t.sections[i.sectionIndex];r&&(this._commitBeforeChange(),ni(t,i.sectionIndex,Sv(r,i.cardIndex,e.detail.config)),this._cardEditorOpen=!1,this._cardEditorTarget=void 0,this._notifyLayoutChanged())}_saveSectionEdit(e){const t=this._view(),i=this._sectionEditorTarget;!t||i===void 0||(this._commitBeforeChange(),Ev(t,i,e.detail.patch),this._sectionEditorOpen=!1,this._sectionEditorTarget=void 0,this._notifyLayoutChanged())}async _persist(){if(!this.lovelace)return;await Iv(this.lovelace);const e=kp(this.lovelace);this.flow.connection.trackAddresses(e),this.flow.connection.query(e)}_enterEditMode(){this._layoutEditEnabled&&(this._editMode=!0,this._undoRedo.reset())}_exitEditMode(){this._editMode=!1,this._undoRedo.reset()}_bindConnection(){var e,t;if((e=this._unsubConnection)==null||e.call(this),this._unsubConnection=void 0,!((t=this.flow)!=null&&t.connection)){this._connected=!1;return}this._connected=this.flow.connected,this._unsubConnection=this.flow.connection.subscribeConnection(i=>{this._connected=i})}render(){var p,f,b,v,_,k,C,O;if(!this.lovelace||!this.flow)return m;const e=this.lovelace.views,t=e[this._activeView],i=this._cardEditorTarget&&t?t.sections[this._cardEditorTarget.sectionIndex]:void 0,r=i&&this._cardEditorTarget?i.cards[this._cardEditorTarget.cardIndex]:void 0,o=this._sectionEditorTarget!==void 0&&t?t.sections[this._sectionEditorTarget]:void 0,n=(t==null?void 0:t.title)??(t==null?void 0:t.path)??this.lovelace.title??"Flow",s=this._headerTitle(n),a=this._panel==="home",l=a&&this._editMode,c=e.length>1,d=a&&(l||c);return h`
      <ha-drawer
        class="shell"
        .type=${this._narrow?"modal":""}
        .open=${this._drawerOpen}
        @hass-drawer-closed=${()=>{this._drawerOpen=!1}}
      >
        <ha-sidebar
          slot="sidebar"
          .activePanel=${this._panel}
          .connected=${this._connected}
          .narrow=${this._narrow}
          @panel-selected=${this._onPanelSelected}
        ></ha-sidebar>

        <div class="main ${this._narrow?"narrow":""} ${l?"edit-mode":""}">
          <header class="header">
            <div class="toolbar">
              <ha-menu-button
                .narrow=${this._narrow}
                .hasNotifications=${!this._connected}
              ></ha-menu-button>
              <div class="main-title">
                ${d&&!l?h`
                      <flow-view-tabs
                        .views=${e}
                        .activeIndex=${this._activeView}
                        @view-selected=${this._onViewSelected}
                      ></flow-view-tabs>
                    `:h`
                      <span>${s}</span>
                      ${l?h`<ha-icon icon="mdi:pencil" class="edit-pencil"></ha-icon>`:m}
                    `}
              </div>
              <div class="action-items">
                <span
                  class="conn-status ${this._connected?"online":"offline"}"
                  title=${this._connected?"已连接到 Flow":"未连接到 Flow（演示模式）"}
                  role="status"
                  aria-live="polite"
                >
                  <ha-svg-icon
                    .path=${this._connected?_f:yf}
                  ></ha-svg-icon>
                </span>
                ${a&&this._layoutEditEnabled?l?h`
                        <ha-icon-button
                          .path=${Pf}
                          label="撤销"
                          .disabled=${!this._undoRedo.canUndo}
                          @click=${this._undo}
                        ></ha-icon-button>
                        <ha-icon-button
                          .path=${Sf}
                          label="重做"
                          .disabled=${!this._undoRedo.canRedo}
                          @click=${this._redo}
                        ></ha-icon-button>
                        <ha-icon-button
                          .path=${ff}
                          label="完成"
                          @click=${this._exitEditMode}
                        ></ha-icon-button>
                      `:h`
                        <ha-icon-button
                          .path=${Ah}
                          label="编辑"
                          @click=${this._enterEditMode}
                        ></ha-icon-button>
                      `:m}
              </div>
            </div>
            ${l?h`
                  <div class="edit-tabs">
                    <flow-view-tabs
                      .views=${e}
                      .activeIndex=${this._activeView}
                      editMode
                      @view-selected=${this._onViewSelected}
                      @view-add=${this._onViewAdd}
                      @view-edit=${this._onViewEdit}
                      @view-delete=${this._onViewDelete}
                    ></flow-view-tabs>
                  </div>
                `:m}
          </header>
          <div class="content">
            ${this._panel==="home"&&t?h`
                  <flow-view
                    .config=${t}
                    .viewIndex=${this._activeView}
                    .editMode=${this._editMode}
                    .layoutRevision=${this._layoutRevision}
                  ></flow-view>
                `:this._panel==="actions"?h`<flow-actions-panel .flow=${this.flow}></flow-actions-panel>`:this._panel==="logs"?h`<flow-logs-panel .wsConnected=${this._connected}></flow-logs-panel>`:this._panel==="media"?h`<flow-media-panel></flow-media-panel>`:this._panel==="config"?h`<flow-settings-panel .flow=${this.flow}></flow-settings-panel>`:m}
          </div>
        </div>
      </ha-drawer>

      <flow-card-picker
          .open=${this._pickerOpen}
          .flow=${this.flow}
          @close=${()=>{this._pickerOpen=!1,this._pickerSection=void 0}}
          @card-picked=${I=>void this._pickCard(I.detail)}
        ></flow-card-picker>

        <flow-card-editor
          .open=${this._cardEditorOpen}
          .config=${r}
          .sectionConfig=${i}
          @close=${()=>{this._cardEditorOpen=!1,this._cardEditorTarget=void 0}}
          @save=${this._saveCardEdit}
        ></flow-card-editor>

        <flow-section-editor
          .open=${this._sectionEditorOpen}
          .config=${o}
          .maxColumns=${(t==null?void 0:t.max_columns)??4}
          @close=${()=>{this._sectionEditorOpen=!1,this._sectionEditorTarget=void 0}}
          @save=${this._saveSectionEdit}
        ></flow-section-editor>

        <flow-view-editor
          .open=${this._viewEditorOpen}
          .isNew=${this._viewEditorIsNew}
          .config=${this._viewEditorIsNew?Fc({title:"新建视图",path:"",icon:"mdi:view-dashboard"}):this.lovelace.views[this._viewEditorTarget??this._activeView]}
          .reservedPaths=${this._reservedViewPaths(this._viewEditorIsNew?void 0:this._viewEditorTarget??this._activeView)}
          @close=${()=>{this._viewEditorOpen=!1,this._viewEditorTarget=void 0,this._viewEditorIsNew=!1}}
          @save=${this._saveViewEdit}
        ></flow-view-editor>

        <flow-confirm-dialog
          .open=${this._confirmOpen}
          .title=${((p=this._confirmParams)==null?void 0:p.title)??""}
          .text=${((f=this._confirmParams)==null?void 0:f.text)??""}
          .confirmText=${((b=this._confirmParams)==null?void 0:b.confirmText)??"删除"}
          .cancelText=${((v=this._confirmParams)==null?void 0:v.cancelText)??"取消"}
          .destructive=${((_=this._confirmParams)==null?void 0:_.destructive)??!1}
          @dialog-result=${this._onConfirmResult}
        ></flow-confirm-dialog>

        <flow-delete-card-dialog
          .open=${this._deleteCardOpen}
          .cardConfig=${this._deleteCardConfig}
          @dialog-result=${this._onDeleteCardResult}
        ></flow-delete-card-dialog>

        <flow-badge-card-dialog
          .open=${this._badgeCardOpen}
          .flow=${((k=this._badgeCardParams)==null?void 0:k.flow)??this.flow}
          .cardConfig=${(C=this._badgeCardParams)==null?void 0:C.cardConfig}
          .title=${((O=this._badgeCardParams)==null?void 0:O.title)??""}
          @dialog-closed=${this._onBadgeCardClosed}
        ></flow-badge-card-dialog>
    `}};z.styles=w`
    :host {
      display: block;
      height: 100vh;
      min-height: 100vh;
      overflow: hidden;
      background: var(--primary-background-color, #fafafa);
      color: var(--primary-text-color, #1e293b);
      font-family: var(--ha-font-family, "Segoe UI", system-ui, sans-serif);
      --ha-sidebar-width: calc(56px + var(--safe-area-inset-left, 0px));
      --ha-top-app-bar-width: calc(100% - var(--ha-sidebar-width));
    }
    :host([expanded]) {
      --ha-sidebar-width: calc(256px + var(--safe-area-inset-left, 0px));
    }
    :host([modal]) {
      --ha-sidebar-width: 0px;
      --ha-top-app-bar-width: 100%;
    }
    ha-drawer {
      flex: 1;
      min-width: 0;
      min-height: 0;
      width: 100%;
      height: 100%;
    }
    .main {
      flex: 1;
      min-width: 0;
      display: flex;
      flex-direction: column;
      min-height: 0;
      height: 100%;
      overflow: hidden;
    }
    .content {
      flex: 1;
      min-width: 0;
      min-height: 0;
      overflow: auto;
      display: flex;
      flex-direction: column;
    }
    .header {
      background-color: var(--app-header-background-color, var(--primary-background-color));
      color: var(--app-header-text-color, var(--primary-text-color));
      border-bottom: var(--app-header-border-bottom, 1px solid var(--divider-color));
      padding-top: var(--safe-area-inset-top, 0px);
      flex-shrink: 0;
      z-index: 4;
    }
    .edit-mode .header {
      background-color: var(--app-header-edit-background-color, #455a64);
      color: var(--app-header-edit-text-color, #fff);
      border-bottom: none;
    }
    .toolbar {
      height: var(--header-height, 56px);
      display: flex;
      align-items: center;
      font-size: 20px;
      padding: 0 12px;
      font-weight: 400;
      box-sizing: border-box;
    }
    .narrow .toolbar {
      padding: 0 4px;
    }
    .main-title {
      margin-inline-start: 24px;
      line-height: 1.4;
      flex-grow: 1;
      min-width: 0;
      display: flex;
      align-items: center;
      gap: 8px;
      overflow: hidden;
      white-space: nowrap;
      text-overflow: ellipsis;
    }
    .main-title flow-view-tabs {
      flex: 1;
      min-width: 0;
      height: 100%;
      align-self: stretch;
    }
    .edit-tabs {
      display: flex;
      align-items: stretch;
      min-height: 48px;
      padding: 0 8px 0 12px;
      border-top: 1px solid rgba(255, 255, 255, 0.12);
      --app-header-text-color: var(--app-header-edit-text-color, #fff);
      --ha-tab-indicator-color: var(--app-header-edit-text-color, #fff);
    }
    .edit-tabs flow-view-tabs {
      flex: 1;
      min-width: 0;
    }
    .narrow .main-title {
      margin-inline-start: 8px;
    }
    .edit-pencil {
      color: var(--accent-color, #ff9800);
      width: 18px;
      height: 18px;
      flex-shrink: 0;
    }
    .action-items {
      display: flex;
      align-items: center;
      flex-shrink: 0;
      white-space: nowrap;
      gap: 0;
    }
    .conn-status {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 40px;
      height: 40px;
      flex-shrink: 0;
      color: inherit;
    }
    .conn-status ha-svg-icon {
      width: 24px;
      height: 24px;
      flex-shrink: 0;
    }
    .conn-status.online {
      color: #2e7d32;
    }
    .conn-status.offline {
      color: #c62828;
    }
    .edit-mode .conn-status.online {
      color: #a5d6a7;
    }
    .edit-mode .conn-status.offline {
      color: #ef9a9a;
    }
  `;F([zf({context:vi}),u({attribute:!1})],z.prototype,"flow",2);F([u({attribute:!1})],z.prototype,"lovelace",2);F([g()],z.prototype,"_activeView",2);F([g()],z.prototype,"_panel",2);F([g()],z.prototype,"_editMode",2);F([g()],z.prototype,"_pickerOpen",2);F([g()],z.prototype,"_pickerSection",2);F([g()],z.prototype,"_cardEditorOpen",2);F([g()],z.prototype,"_cardEditorTarget",2);F([g()],z.prototype,"_sectionEditorOpen",2);F([g()],z.prototype,"_sectionEditorTarget",2);F([g()],z.prototype,"_viewEditorOpen",2);F([g()],z.prototype,"_viewEditorIsNew",2);F([g()],z.prototype,"_viewEditorTarget",2);F([g()],z.prototype,"_confirmOpen",2);F([g()],z.prototype,"_confirmParams",2);F([g()],z.prototype,"_deleteCardOpen",2);F([g()],z.prototype,"_deleteCardConfig",2);F([g()],z.prototype,"_badgeCardOpen",2);F([g()],z.prototype,"_badgeCardParams",2);F([g()],z.prototype,"_layoutEditEnabled",2);F([g()],z.prototype,"_layoutRevision",2);F([g()],z.prototype,"_narrow",2);F([g()],z.prototype,"_drawerOpen",2);F([g()],z.prototype,"_sidebarExpanded",2);F([g()],z.prototype,"_connected",2);z=F([x("flow-app")],z);class hw{constructor(){this.states=new Map,this.globalListeners=new Set,this.addressListeners=new Map}getStatesRecord(){const t={};return this.states.forEach((i,r)=>{t[r]=i}),t}getState(t){return this.states.get(t)}applyUpdate(t,i,r={}){var s;const o=this.states.get(t),n={address:t,state:i,attributes:{...o==null?void 0:o.attributes,...r},last_changed:Date.now()};this.states.set(t,n),(s=this.addressListeners.get(t))==null||s.forEach(a=>a(n)),this.notifyGlobal()}subscribe(t,i){this.addressListeners.has(t)||this.addressListeners.set(t,new Set),this.addressListeners.get(t).add(i);const r=this.states.get(t);return r&&i(r),()=>{var o;return(o=this.addressListeners.get(t))==null?void 0:o.delete(i)}}subscribeAll(t){return this.globalListeners.add(t),t(this.getStatesRecord()),()=>this.globalListeners.delete(t)}notifyGlobal(){const t=this.getStatesRecord();this.globalListeners.forEach(i=>i(t))}}class pw{constructor(t,i){this.url=i,this.connected=!1,this.ws=null,this.reconnectTimer=null,this.heartbeatTimer=null,this.connectionListeners=new Set,this.stateListeners=new Set,this.subscribedAddresses=new Set,this.store=t,this.store.subscribeAll(r=>{this.stateListeners.forEach(o=>o(r))})}connect(){this.ws||(this.ws=new WebSocket(this.url),this.ws.onopen=()=>{this.connected=!0,this.notifyConnection(),this.startHeartbeat(),this.subscribedAddresses.size>0&&this.query([...this.subscribedAddresses])},this.ws.onmessage=t=>{try{const i=JSON.parse(String(t.data));if(i.heartbeat)return;if(i.event==="actions_changed"){document.dispatchEvent(new CustomEvent("flow-actions-changed",{detail:i}));return}if(i.event==="log"){document.dispatchEvent(new CustomEvent("flow-log-entry",{detail:{seq:Number(i.seq??0),timestamp:String(i.timestamp??""),level:String(i.level??"Info"),message:String(i.message??"")}}));return}const r=i.address||i.addr;if(!r)return;this.store.applyUpdate(r,i.value),document.dispatchEvent(new CustomEvent("flow-ws-message",{detail:{address:r,value:i.value,raw:i}}))}catch{}},this.ws.onclose=()=>{this.connected=!1,this.ws=null,this.stopHeartbeat(),this.notifyConnection(),this.scheduleReconnect()},this.ws.onerror=()=>{this.connected=!1,this.notifyConnection()})}subscribeStates(t){return this.stateListeners.add(t),t(this.store.getStatesRecord()),()=>this.stateListeners.delete(t)}subscribeConnection(t){return this.connectionListeners.add(t),t(this.connected),()=>this.connectionListeners.delete(t)}trackAddresses(t){const i=t.filter(Boolean);i.forEach(r=>this.subscribedAddresses.add(r)),this.connected&&i.length>0&&this.query(i)}sendCommand(t,i){!this.ws||this.ws.readyState!==WebSocket.OPEN||this.ws.send(JSON.stringify({address:t,value:i}))}query(t){if(!this.ws||this.ws.readyState!==WebSocket.OPEN||t.length===0)return;const i=128;for(let r=0;r<t.length;r+=i)this.ws.send(JSON.stringify({query:t.slice(r,r+i)}))}notifyConnection(){this.connectionListeners.forEach(t=>t(this.connected))}scheduleReconnect(){this.reconnectTimer||(this.reconnectTimer=setTimeout(()=>{this.reconnectTimer=null,this.connect()},3e3))}startHeartbeat(){this.stopHeartbeat(),this.heartbeatTimer=setInterval(()=>{!this.ws||this.ws.readyState!==WebSocket.OPEN||this.ws.send(JSON.stringify({heartbeat:!0,t:Date.now()}))},25e3)}stopHeartbeat(){this.heartbeatTimer&&(clearInterval(this.heartbeatTimer),this.heartbeatTimer=null)}}function uw(e){const t={"/demo/floor_lamp":!0,"/demo/spotlights":70,"/demo/coffee":!1,"/demo/fridge":!0,"/demo/dishwasher":!1,"/demo/power":797.86,"/demo/voltage":235.61,"/demo/co2":458,"/demo/temperature":{value:10.2,attributes:{unit_of_measurement:"°C"}},"/demo/humidity":{value:56,attributes:{unit_of_measurement:"%"}},"/demo/string":{value:"Intro_Loop_v3.mp4",attributes:{friendly_name:"当前素材",icon:"mdi:filmstrip"}},"/demo/brightness":70,"/demo/path_progress":{value:.35,attributes:{friendly_name:"轨迹进度",icon:"mdi:robot-industrial"}},"/demo/trigger":!1,"/demo/switch":!1,"/demo/climate":{value:23,attributes:{friendly_name:"Upstairs",current_temperature:21.7}},"/demo/gain":{value:-18,attributes:{friendly_name:"输入增益",unit_of_measurement:"dB"}},"/demo/rgba":{value:[.12,.53,.9,1],attributes:{friendly_name:"RGBA",icon:"mdi:palette"}},"/demo/hsv":{value:[.58,.87,.9],attributes:{friendly_name:"HSV",icon:"mdi:palette-swatch"}},"/demo/faders":{value:[.25,.5,.75,.4],attributes:{friendly_name:"多维推杆",icon:"mdi:tune-vertical-variant"}},"/demo/xy":{value:[.5,.5],attributes:{friendly_name:"XY Pad",icon:"mdi:axis-arrow"}},"/demo/media":{value:!0,attributes:{friendly_name:"媒体播放器"}},"/demo/status_ws":{value:!0,attributes:{friendly_name:"connected",icon:"mdi:lan-connect"}},"/demo/status_tcp":{value:!0,attributes:{friendly_name:"tcp连接",icon:"mdi:lan-connect"}},"/demo/status_udp":{value:!0,attributes:{friendly_name:"connected",icon:"mdi:lan-connect"}},"/demo/status_osc":{value:!0,attributes:{friendly_name:"connected",icon:"mdi:lan-connect"}},"/demo/kitchen_shutter":{value:"open",attributes:{friendly_name:"Kitchen shutter",current_position:3}},"/demo/line3d":{value:"",attributes:{points:o0(120)}},"/demo/scatter3d":{value:"",attributes:{points:n0(80)}},"/demo/line2d":{value:"",attributes:{points:i0(80)}},"/demo/scatter2d":{value:"",attributes:{points:r0(60)}}};Object.entries(t).forEach(([i,r])=>{typeof r=="object"&&r!==null&&"value"in r?e.applyUpdate(i,r.value,r.attributes??{}):e.applyUpdate(i,r)})}async function fw(){const e=new hw,t=location.protocol==="https:"?"wss:":"ws:",i=new pw(e,`${t}//${location.host}/ws`),r=T_(e,i);uw(e);const o=await Dv(),n=kp(o),s=document.querySelector("flow-app");s instanceof HTMLElement&&(s.flow=r,s.lovelace=o,i.trackAddresses(n),i.connect(),i.subscribeStates(()=>{s.requestUpdate()}),i.subscribeConnection(()=>{s.requestUpdate()}),document.addEventListener("visibilitychange",()=>{document.visibilityState==="visible"&&i.query(n)}))}fw().catch(e=>{console.error("Flow bootstrap failed",e)});export{m as A,Lw as C,y as a,h as b,Ke as c,Un as d,mi as e,Ow as f,He as g,w as i,u as n,P as o,g as r,x as t};
