const __vite__mapDeps=(i,m=__vite__mapDeps,d=(m.f||(m.f=["./hui-line3d-card-C-h9W1E5.js","./chart-3d-card-shared-BgfkzOBX.js","./echarts-DsuMhSbC.js","./mdi-icons-CYzTwnk_.js","./hui-scatter3d-card-DjuYvItv.js","./hui-line2d-card-DMoeVuwq.js","./chart-2d-card-shared-C7GKXeXl.js","./hui-scatter2d-card-BLtS0gsb.js","./hui-bar-card-CMEZKviK.js"])))=>i.map(i=>d[i]);
var hu=Object.defineProperty;var uu=(e,t,i)=>t in e?hu(e,t,{enumerable:!0,configurable:!0,writable:!0,value:i}):e[t]=i;var L=(e,t,i)=>uu(e,typeof t!="symbol"?t+"":t,i);import{mdiMagnify as td,mdiCursorMove as pu,mdiDelete as fu,mdiPlusCircleMultipleOutline as gu,mdiContentPaste as mu,mdiContentCut as vu,mdiContentCopy as bu,mdiDotsVertical as _u,mdiText as yu,mdiVolumeOff as xu,mdiVolumeHigh as wu,mdiTuneVertical as $u,mdiGauge as ku,mdiViewGrid as Su,mdiFormatTitle as Cu,mdiBrightness6 as Eu,mdiGestureTapButton as Pu,mdiToggleSwitch as Tu,mdiAlertCircle as Ou,mdiEye as Au,mdiHelpCircle as vn,mdiLightbulb as Du,mdiRedo as id,mdiUndo as rd,mdiPencil as ba,mdiStop as Iu,mdiPlay as Mu,mdiLinkVariant as zu,mdiAxisArrow as Nu,mdiPalette as Ru,mdiHomeThermometer as Lu,mdiClockOutline as Bu,mdiCastAudio as Fu,mdiBell as Hu,mdiMenuOpen as od,mdiMenu as _a,mdiCog as nd,mdiViewDashboard as Uu,mdiMoleculeCo2 as ju,mdiSineWave as Vu,mdiFloorPlan as qu,mdiFlash as Gu,mdiLightningBolt as Xu,mdiDishwasher as Yu,mdiFridge as Wu,mdiCoffee as Ku,mdiSilverwareForkKnife as Zu,mdiAirConditioner as Qu,mdiWindowShutter as Ju,mdiSpotlightBeam as ep,mdiFloorLamp as tp,mdiWaterPercent as ip,mdiThermometer as rp,mdiSofa as op,mdiHandWave as np,mdiHome as sd,mdiPlus as sp,mdiRestore as ap,mdiClose as lp,mdiLanConnect as cp,mdiLanDisconnect as dp,mdiCheck as hp}from"./mdi-icons-CYzTwnk_.js";(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const o of document.querySelectorAll('link[rel="modulepreload"]'))r(o);new MutationObserver(o=>{for(const n of o)if(n.type==="childList")for(const s of n.addedNodes)s.tagName==="LINK"&&s.rel==="modulepreload"&&r(s)}).observe(document,{childList:!0,subtree:!0});function i(o){const n={};return o.integrity&&(n.integrity=o.integrity),o.referrerPolicy&&(n.referrerPolicy=o.referrerPolicy),o.crossOrigin==="use-credentials"?n.credentials="include":o.crossOrigin==="anonymous"?n.credentials="omit":n.credentials="same-origin",n}function r(o){if(o.ep)return;o.ep=!0;const n=i(o);fetch(o.href,n)}})();const up="modulepreload",pp=function(e,t){return new URL(e,t).href},Tl={},Wi=function(t,i,r){let o=Promise.resolve();if(i&&i.length>0){const s=document.getElementsByTagName("link"),a=document.querySelector("meta[property=csp-nonce]"),l=(a==null?void 0:a.nonce)||(a==null?void 0:a.getAttribute("nonce"));o=Promise.allSettled(i.map(c=>{if(c=pp(c,r),c in Tl)return;Tl[c]=!0;const d=c.endsWith(".css"),h=d?'[rel="stylesheet"]':"";if(!!r)for(let g=s.length-1;g>=0;g--){const _=s[g];if(_.href===c&&(!d||_.rel==="stylesheet"))return}else if(document.querySelector(`link[href="${c}"]${h}`))return;const v=document.createElement("link");if(v.rel=d?"stylesheet":up,d||(v.as="script"),v.crossOrigin="",v.href=c,l&&v.setAttribute("nonce",l),document.head.appendChild(v),d)return new Promise((g,_)=>{v.addEventListener("load",g),v.addEventListener("error",()=>_(new Error(`Unable to preload CSS for ${c}`)))})}))}function n(s){const a=new Event("vite:preloadError",{cancelable:!0});if(a.payload=s,window.dispatchEvent(a),!a.defaultPrevented)throw s}return o.then(s=>{for(const a of s||[])a.status==="rejected"&&n(a.reason);return t().catch(n)})},ts=new Map;function G(e){ts.set(e.type,e)}function fp(e){return ts.get(e)}function gp(){return[...ts.values()]}function Ol(e){return Object.hasOwn(e,"constructor")&&typeof e.constructor=="function"}async function ya(e){const t=ts.get(e);if(t)return Ol(t)?t.constructor:(t.loader&&await t.loader(),Ol(t)?t.constructor:void 0)}function mp(){return gp().filter(e=>e.type!=="error"&&e.type!=="color")}/**
 * @license
 * Copyright 2019 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const bn=globalThis,xa=bn.ShadowRoot&&(bn.ShadyCSS===void 0||bn.ShadyCSS.nativeShadow)&&"adoptedStyleSheets"in Document.prototype&&"replace"in CSSStyleSheet.prototype,wa=Symbol(),Al=new WeakMap;let ad=class{constructor(t,i,r){if(this._$cssResult$=!0,r!==wa)throw Error("CSSResult is not constructable. Use `unsafeCSS` or `css` instead.");this.cssText=t,this.t=i}get styleSheet(){let t=this.o;const i=this.t;if(xa&&t===void 0){const r=i!==void 0&&i.length===1;r&&(t=Al.get(i)),t===void 0&&((this.o=t=new CSSStyleSheet).replaceSync(this.cssText),r&&Al.set(i,t))}return t}toString(){return this.cssText}};const vp=e=>new ad(typeof e=="string"?e:e+"",void 0,wa),$=(e,...t)=>{const i=e.length===1?e[0]:t.reduce((r,o,n)=>r+(s=>{if(s._$cssResult$===!0)return s.cssText;if(typeof s=="number")return s;throw Error("Value passed to 'css' function must be a 'css' function result: "+s+". Use 'unsafeCSS' to pass non-literal values, but take care to ensure page security.")})(o)+e[n+1],e[0]);return new ad(i,e,wa)},bp=(e,t)=>{if(xa)e.adoptedStyleSheets=t.map(i=>i instanceof CSSStyleSheet?i:i.styleSheet);else for(const i of t){const r=document.createElement("style"),o=bn.litNonce;o!==void 0&&r.setAttribute("nonce",o),r.textContent=i.cssText,e.appendChild(r)}},Dl=xa?e=>e:e=>e instanceof CSSStyleSheet?(t=>{let i="";for(const r of t.cssRules)i+=r.cssText;return vp(i)})(e):e;/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const{is:_p,defineProperty:yp,getOwnPropertyDescriptor:xp,getOwnPropertyNames:wp,getOwnPropertySymbols:$p,getPrototypeOf:kp}=Object,qt=globalThis,Il=qt.trustedTypes,Sp=Il?Il.emptyScript:"",ws=qt.reactiveElementPolyfillSupport,eo=(e,t)=>e,Pn={toAttribute(e,t){switch(t){case Boolean:e=e?Sp:null;break;case Object:case Array:e=e==null?e:JSON.stringify(e)}return e},fromAttribute(e,t){let i=e;switch(t){case Boolean:i=e!==null;break;case Number:i=e===null?null:Number(e);break;case Object:case Array:try{i=JSON.parse(e)}catch{i=null}}return i}},$a=(e,t)=>!_p(e,t),Ml={attribute:!0,type:String,converter:Pn,reflect:!1,useDefault:!1,hasChanged:$a};Symbol.metadata??(Symbol.metadata=Symbol("metadata")),qt.litPropertyMetadata??(qt.litPropertyMetadata=new WeakMap);let hi=class extends HTMLElement{static addInitializer(t){this._$Ei(),(this.l??(this.l=[])).push(t)}static get observedAttributes(){return this.finalize(),this._$Eh&&[...this._$Eh.keys()]}static createProperty(t,i=Ml){if(i.state&&(i.attribute=!1),this._$Ei(),this.prototype.hasOwnProperty(t)&&((i=Object.create(i)).wrapped=!0),this.elementProperties.set(t,i),!i.noAccessor){const r=Symbol(),o=this.getPropertyDescriptor(t,r,i);o!==void 0&&yp(this.prototype,t,o)}}static getPropertyDescriptor(t,i,r){const{get:o,set:n}=xp(this.prototype,t)??{get(){return this[i]},set(s){this[i]=s}};return{get:o,set(s){const a=o==null?void 0:o.call(this);n==null||n.call(this,s),this.requestUpdate(t,a,r)},configurable:!0,enumerable:!0}}static getPropertyOptions(t){return this.elementProperties.get(t)??Ml}static _$Ei(){if(this.hasOwnProperty(eo("elementProperties")))return;const t=kp(this);t.finalize(),t.l!==void 0&&(this.l=[...t.l]),this.elementProperties=new Map(t.elementProperties)}static finalize(){if(this.hasOwnProperty(eo("finalized")))return;if(this.finalized=!0,this._$Ei(),this.hasOwnProperty(eo("properties"))){const i=this.properties,r=[...wp(i),...$p(i)];for(const o of r)this.createProperty(o,i[o])}const t=this[Symbol.metadata];if(t!==null){const i=litPropertyMetadata.get(t);if(i!==void 0)for(const[r,o]of i)this.elementProperties.set(r,o)}this._$Eh=new Map;for(const[i,r]of this.elementProperties){const o=this._$Eu(i,r);o!==void 0&&this._$Eh.set(o,i)}this.elementStyles=this.finalizeStyles(this.styles)}static finalizeStyles(t){const i=[];if(Array.isArray(t)){const r=new Set(t.flat(1/0).reverse());for(const o of r)i.unshift(Dl(o))}else t!==void 0&&i.push(Dl(t));return i}static _$Eu(t,i){const r=i.attribute;return r===!1?void 0:typeof r=="string"?r:typeof t=="string"?t.toLowerCase():void 0}constructor(){super(),this._$Ep=void 0,this.isUpdatePending=!1,this.hasUpdated=!1,this._$Em=null,this._$Ev()}_$Ev(){var t;this._$ES=new Promise(i=>this.enableUpdating=i),this._$AL=new Map,this._$E_(),this.requestUpdate(),(t=this.constructor.l)==null||t.forEach(i=>i(this))}addController(t){var i;(this._$EO??(this._$EO=new Set)).add(t),this.renderRoot!==void 0&&this.isConnected&&((i=t.hostConnected)==null||i.call(t))}removeController(t){var i;(i=this._$EO)==null||i.delete(t)}_$E_(){const t=new Map,i=this.constructor.elementProperties;for(const r of i.keys())this.hasOwnProperty(r)&&(t.set(r,this[r]),delete this[r]);t.size>0&&(this._$Ep=t)}createRenderRoot(){const t=this.shadowRoot??this.attachShadow(this.constructor.shadowRootOptions);return bp(t,this.constructor.elementStyles),t}connectedCallback(){var t;this.renderRoot??(this.renderRoot=this.createRenderRoot()),this.enableUpdating(!0),(t=this._$EO)==null||t.forEach(i=>{var r;return(r=i.hostConnected)==null?void 0:r.call(i)})}enableUpdating(t){}disconnectedCallback(){var t;(t=this._$EO)==null||t.forEach(i=>{var r;return(r=i.hostDisconnected)==null?void 0:r.call(i)})}attributeChangedCallback(t,i,r){this._$AK(t,r)}_$ET(t,i){var n;const r=this.constructor.elementProperties.get(t),o=this.constructor._$Eu(t,r);if(o!==void 0&&r.reflect===!0){const s=(((n=r.converter)==null?void 0:n.toAttribute)!==void 0?r.converter:Pn).toAttribute(i,r.type);this._$Em=t,s==null?this.removeAttribute(o):this.setAttribute(o,s),this._$Em=null}}_$AK(t,i){var n,s;const r=this.constructor,o=r._$Eh.get(t);if(o!==void 0&&this._$Em!==o){const a=r.getPropertyOptions(o),l=typeof a.converter=="function"?{fromAttribute:a.converter}:((n=a.converter)==null?void 0:n.fromAttribute)!==void 0?a.converter:Pn;this._$Em=o;const c=l.fromAttribute(i,a.type);this[o]=c??((s=this._$Ej)==null?void 0:s.get(o))??c,this._$Em=null}}requestUpdate(t,i,r,o=!1,n){var s;if(t!==void 0){const a=this.constructor;if(o===!1&&(n=this[t]),r??(r=a.getPropertyOptions(t)),!((r.hasChanged??$a)(n,i)||r.useDefault&&r.reflect&&n===((s=this._$Ej)==null?void 0:s.get(t))&&!this.hasAttribute(a._$Eu(t,r))))return;this.C(t,i,r)}this.isUpdatePending===!1&&(this._$ES=this._$EP())}C(t,i,{useDefault:r,reflect:o,wrapped:n},s){r&&!(this._$Ej??(this._$Ej=new Map)).has(t)&&(this._$Ej.set(t,s??i??this[t]),n!==!0||s!==void 0)||(this._$AL.has(t)||(this.hasUpdated||r||(i=void 0),this._$AL.set(t,i)),o===!0&&this._$Em!==t&&(this._$Eq??(this._$Eq=new Set)).add(t))}async _$EP(){this.isUpdatePending=!0;try{await this._$ES}catch(i){Promise.reject(i)}const t=this.scheduleUpdate();return t!=null&&await t,!this.isUpdatePending}scheduleUpdate(){return this.performUpdate()}performUpdate(){var r;if(!this.isUpdatePending)return;if(!this.hasUpdated){if(this.renderRoot??(this.renderRoot=this.createRenderRoot()),this._$Ep){for(const[n,s]of this._$Ep)this[n]=s;this._$Ep=void 0}const o=this.constructor.elementProperties;if(o.size>0)for(const[n,s]of o){const{wrapped:a}=s,l=this[n];a!==!0||this._$AL.has(n)||l===void 0||this.C(n,void 0,s,l)}}let t=!1;const i=this._$AL;try{t=this.shouldUpdate(i),t?(this.willUpdate(i),(r=this._$EO)==null||r.forEach(o=>{var n;return(n=o.hostUpdate)==null?void 0:n.call(o)}),this.update(i)):this._$EM()}catch(o){throw t=!1,this._$EM(),o}t&&this._$AE(i)}willUpdate(t){}_$AE(t){var i;(i=this._$EO)==null||i.forEach(r=>{var o;return(o=r.hostUpdated)==null?void 0:o.call(r)}),this.hasUpdated||(this.hasUpdated=!0,this.firstUpdated(t)),this.updated(t)}_$EM(){this._$AL=new Map,this.isUpdatePending=!1}get updateComplete(){return this.getUpdateComplete()}getUpdateComplete(){return this._$ES}shouldUpdate(t){return!0}update(t){this._$Eq&&(this._$Eq=this._$Eq.forEach(i=>this._$ET(i,this[i]))),this._$EM()}updated(t){}firstUpdated(t){}};hi.elementStyles=[],hi.shadowRootOptions={mode:"open"},hi[eo("elementProperties")]=new Map,hi[eo("finalized")]=new Map,ws==null||ws({ReactiveElement:hi}),(qt.reactiveElementVersions??(qt.reactiveElementVersions=[])).push("2.1.2");/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const to=globalThis,zl=e=>e,Tn=to.trustedTypes,Nl=Tn?Tn.createPolicy("lit-html",{createHTML:e=>e}):void 0,ld="$lit$",Ft=`lit$${Math.random().toFixed(9).slice(2)}$`,cd="?"+Ft,Cp=`<${cd}>`,_i=document,po=()=>_i.createComment(""),fo=e=>e===null||typeof e!="object"&&typeof e!="function",ka=Array.isArray,Ep=e=>ka(e)||typeof(e==null?void 0:e[Symbol.iterator])=="function",$s=`[ 	
\f\r]`,Ur=/<(?:(!--|\/[^a-zA-Z])|(\/?[a-zA-Z][^>\s]*)|(\/?$))/g,Rl=/-->/g,Ll=/>/g,si=RegExp(`>|${$s}(?:([^\\s"'>=/]+)(${$s}*=${$s}*(?:[^ 	
\f\r"'\`<>=]|("|')|))|$)`,"g"),Bl=/'/g,Fl=/"/g,dd=/^(?:script|style|textarea|title)$/i,Pp=e=>(t,...i)=>({_$litType$:e,strings:t,values:i}),u=Pp(1),Et=Symbol.for("lit-noChange"),m=Symbol.for("lit-nothing"),Hl=new WeakMap,pi=_i.createTreeWalker(_i,129);function hd(e,t){if(!ka(e)||!e.hasOwnProperty("raw"))throw Error("invalid template strings array");return Nl!==void 0?Nl.createHTML(t):t}const Tp=(e,t)=>{const i=e.length-1,r=[];let o,n=t===2?"<svg>":t===3?"<math>":"",s=Ur;for(let a=0;a<i;a++){const l=e[a];let c,d,h=-1,f=0;for(;f<l.length&&(s.lastIndex=f,d=s.exec(l),d!==null);)f=s.lastIndex,s===Ur?d[1]==="!--"?s=Rl:d[1]!==void 0?s=Ll:d[2]!==void 0?(dd.test(d[2])&&(o=RegExp("</"+d[2],"g")),s=si):d[3]!==void 0&&(s=si):s===si?d[0]===">"?(s=o??Ur,h=-1):d[1]===void 0?h=-2:(h=s.lastIndex-d[2].length,c=d[1],s=d[3]===void 0?si:d[3]==='"'?Fl:Bl):s===Fl||s===Bl?s=si:s===Rl||s===Ll?s=Ur:(s=si,o=void 0);const v=s===si&&e[a+1].startsWith("/>")?" ":"";n+=s===Ur?l+Cp:h>=0?(r.push(c),l.slice(0,h)+ld+l.slice(h)+Ft+v):l+Ft+(h===-2?a:v)}return[hd(e,n+(e[i]||"<?>")+(t===2?"</svg>":t===3?"</math>":"")),r]};let Gs=class ud{constructor({strings:t,_$litType$:i},r){let o;this.parts=[];let n=0,s=0;const a=t.length-1,l=this.parts,[c,d]=Tp(t,i);if(this.el=ud.createElement(c,r),pi.currentNode=this.el.content,i===2||i===3){const h=this.el.content.firstChild;h.replaceWith(...h.childNodes)}for(;(o=pi.nextNode())!==null&&l.length<a;){if(o.nodeType===1){if(o.hasAttributes())for(const h of o.getAttributeNames())if(h.endsWith(ld)){const f=d[s++],v=o.getAttribute(h).split(Ft),g=/([.?@])?(.*)/.exec(f);l.push({type:1,index:n,name:g[2],strings:v,ctor:g[1]==="."?Ap:g[1]==="?"?Dp:g[1]==="@"?Ip:is}),o.removeAttribute(h)}else h.startsWith(Ft)&&(l.push({type:6,index:n}),o.removeAttribute(h));if(dd.test(o.tagName)){const h=o.textContent.split(Ft),f=h.length-1;if(f>0){o.textContent=Tn?Tn.emptyScript:"";for(let v=0;v<f;v++)o.append(h[v],po()),pi.nextNode(),l.push({type:2,index:++n});o.append(h[f],po())}}}else if(o.nodeType===8)if(o.data===cd)l.push({type:2,index:n});else{let h=-1;for(;(h=o.data.indexOf(Ft,h+1))!==-1;)l.push({type:7,index:n}),h+=Ft.length-1}n++}}static createElement(t,i){const r=_i.createElement("template");return r.innerHTML=t,r}};function rr(e,t,i=e,r){var s,a;if(t===Et)return t;let o=r!==void 0?(s=i._$Co)==null?void 0:s[r]:i._$Cl;const n=fo(t)?void 0:t._$litDirective$;return(o==null?void 0:o.constructor)!==n&&((a=o==null?void 0:o._$AO)==null||a.call(o,!1),n===void 0?o=void 0:(o=new n(e),o._$AT(e,i,r)),r!==void 0?(i._$Co??(i._$Co=[]))[r]=o:i._$Cl=o),o!==void 0&&(t=rr(e,o._$AS(e,t.values),o,r)),t}let Op=class{constructor(t,i){this._$AV=[],this._$AN=void 0,this._$AD=t,this._$AM=i}get parentNode(){return this._$AM.parentNode}get _$AU(){return this._$AM._$AU}u(t){const{el:{content:i},parts:r}=this._$AD,o=((t==null?void 0:t.creationScope)??_i).importNode(i,!0);pi.currentNode=o;let n=pi.nextNode(),s=0,a=0,l=r[0];for(;l!==void 0;){if(s===l.index){let c;l.type===2?c=new Pr(n,n.nextSibling,this,t):l.type===1?c=new l.ctor(n,l.name,l.strings,this,t):l.type===6&&(c=new Mp(n,this,t)),this._$AV.push(c),l=r[++a]}s!==(l==null?void 0:l.index)&&(n=pi.nextNode(),s++)}return pi.currentNode=_i,o}p(t){let i=0;for(const r of this._$AV)r!==void 0&&(r.strings!==void 0?(r._$AI(t,r,i),i+=r.strings.length-2):r._$AI(t[i])),i++}};class Pr{get _$AU(){var t;return((t=this._$AM)==null?void 0:t._$AU)??this._$Cv}constructor(t,i,r,o){this.type=2,this._$AH=m,this._$AN=void 0,this._$AA=t,this._$AB=i,this._$AM=r,this.options=o,this._$Cv=(o==null?void 0:o.isConnected)??!0}get parentNode(){let t=this._$AA.parentNode;const i=this._$AM;return i!==void 0&&(t==null?void 0:t.nodeType)===11&&(t=i.parentNode),t}get startNode(){return this._$AA}get endNode(){return this._$AB}_$AI(t,i=this){t=rr(this,t,i),fo(t)?t===m||t==null||t===""?(this._$AH!==m&&this._$AR(),this._$AH=m):t!==this._$AH&&t!==Et&&this._(t):t._$litType$!==void 0?this.$(t):t.nodeType!==void 0?this.T(t):Ep(t)?this.k(t):this._(t)}O(t){return this._$AA.parentNode.insertBefore(t,this._$AB)}T(t){this._$AH!==t&&(this._$AR(),this._$AH=this.O(t))}_(t){this._$AH!==m&&fo(this._$AH)?this._$AA.nextSibling.data=t:this.T(_i.createTextNode(t)),this._$AH=t}$(t){var n;const{values:i,_$litType$:r}=t,o=typeof r=="number"?this._$AC(t):(r.el===void 0&&(r.el=Gs.createElement(hd(r.h,r.h[0]),this.options)),r);if(((n=this._$AH)==null?void 0:n._$AD)===o)this._$AH.p(i);else{const s=new Op(o,this),a=s.u(this.options);s.p(i),this.T(a),this._$AH=s}}_$AC(t){let i=Hl.get(t.strings);return i===void 0&&Hl.set(t.strings,i=new Gs(t)),i}k(t){ka(this._$AH)||(this._$AH=[],this._$AR());const i=this._$AH;let r,o=0;for(const n of t)o===i.length?i.push(r=new Pr(this.O(po()),this.O(po()),this,this.options)):r=i[o],r._$AI(n),o++;o<i.length&&(this._$AR(r&&r._$AB.nextSibling,o),i.length=o)}_$AR(t=this._$AA.nextSibling,i){var r;for((r=this._$AP)==null?void 0:r.call(this,!1,!0,i);t!==this._$AB;){const o=zl(t).nextSibling;zl(t).remove(),t=o}}setConnected(t){var i;this._$AM===void 0&&(this._$Cv=t,(i=this._$AP)==null||i.call(this,t))}}let is=class{get tagName(){return this.element.tagName}get _$AU(){return this._$AM._$AU}constructor(t,i,r,o,n){this.type=1,this._$AH=m,this._$AN=void 0,this.element=t,this.name=i,this._$AM=o,this.options=n,r.length>2||r[0]!==""||r[1]!==""?(this._$AH=Array(r.length-1).fill(new String),this.strings=r):this._$AH=m}_$AI(t,i=this,r,o){const n=this.strings;let s=!1;if(n===void 0)t=rr(this,t,i,0),s=!fo(t)||t!==this._$AH&&t!==Et,s&&(this._$AH=t);else{const a=t;let l,c;for(t=n[0],l=0;l<n.length-1;l++)c=rr(this,a[r+l],i,l),c===Et&&(c=this._$AH[l]),s||(s=!fo(c)||c!==this._$AH[l]),c===m?t=m:t!==m&&(t+=(c??"")+n[l+1]),this._$AH[l]=c}s&&!o&&this.j(t)}j(t){t===m?this.element.removeAttribute(this.name):this.element.setAttribute(this.name,t??"")}},Ap=class extends is{constructor(){super(...arguments),this.type=3}j(t){this.element[this.name]=t===m?void 0:t}},Dp=class extends is{constructor(){super(...arguments),this.type=4}j(t){this.element.toggleAttribute(this.name,!!t&&t!==m)}},Ip=class extends is{constructor(t,i,r,o,n){super(t,i,r,o,n),this.type=5}_$AI(t,i=this){if((t=rr(this,t,i,0)??m)===Et)return;const r=this._$AH,o=t===m&&r!==m||t.capture!==r.capture||t.once!==r.once||t.passive!==r.passive,n=t!==m&&(r===m||o);o&&this.element.removeEventListener(this.name,this,r),n&&this.element.addEventListener(this.name,this,t),this._$AH=t}handleEvent(t){var i;typeof this._$AH=="function"?this._$AH.call(((i=this.options)==null?void 0:i.host)??this.element,t):this._$AH.handleEvent(t)}},Mp=class{constructor(t,i,r){this.element=t,this.type=6,this._$AN=void 0,this._$AM=i,this.options=r}get _$AU(){return this._$AM._$AU}_$AI(t){rr(this,t)}};const zp={I:Pr},ks=to.litHtmlPolyfillSupport;ks==null||ks(Gs,Pr),(to.litHtmlVersions??(to.litHtmlVersions=[])).push("3.3.3");const Np=(e,t,i)=>{const r=(i==null?void 0:i.renderBefore)??t;let o=r._$litPart$;if(o===void 0){const n=(i==null?void 0:i.renderBefore)??null;r._$litPart$=o=new Pr(t.insertBefore(po(),n),n,void 0,i??{})}return o._$AI(e),o};/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const gi=globalThis;let y=class extends hi{constructor(){super(...arguments),this.renderOptions={host:this},this._$Do=void 0}createRenderRoot(){var i;const t=super.createRenderRoot();return(i=this.renderOptions).renderBefore??(i.renderBefore=t.firstChild),t}update(t){const i=this.render();this.hasUpdated||(this.renderOptions.isConnected=this.isConnected),super.update(t),this._$Do=Np(i,this.renderRoot,this.renderOptions)}connectedCallback(){var t;super.connectedCallback(),(t=this._$Do)==null||t.setConnected(!0)}disconnectedCallback(){var t;super.disconnectedCallback(),(t=this._$Do)==null||t.setConnected(!1)}render(){return Et}};var ed;y._$litElement$=!0,y.finalized=!0,(ed=gi.litElementHydrateSupport)==null||ed.call(gi,{LitElement:y});const Ss=gi.litElementPolyfillSupport;Ss==null||Ss({LitElement:y});(gi.litElementVersions??(gi.litElementVersions=[])).push("4.2.2");/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const w=e=>(t,i)=>{i!==void 0?i.addInitializer(()=>{customElements.define(e,t)}):customElements.define(e,t)};/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const Rp={attribute:!0,type:String,converter:Pn,reflect:!1,hasChanged:$a},Lp=(e=Rp,t,i)=>{const{kind:r,metadata:o}=i;let n=globalThis.litPropertyMetadata.get(o);if(n===void 0&&globalThis.litPropertyMetadata.set(o,n=new Map),r==="setter"&&((e=Object.create(e)).wrapped=!0),n.set(i.name,e),r==="accessor"){const{name:s}=i;return{set(a){const l=t.get.call(this);t.set.call(this,a),this.requestUpdate(s,l,e,!0,a)},init(a){return a!==void 0&&this.C(s,void 0,e,a),a}}}if(r==="setter"){const{name:s}=i;return function(a){const l=this[s];t.call(this,a),this.requestUpdate(s,l,e,!0,a)}}throw Error("Unsupported decorator location: "+r)};function p(e){return(t,i)=>typeof i=="object"?Lp(e,t,i):((r,o,n)=>{const s=o.hasOwnProperty(n);return o.constructor.createProperty(n,r),s?Object.getOwnPropertyDescriptor(o,n):void 0})(e,t,i)}/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */function b(e){return p({...e,state:!0,attribute:!1})}/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const Bp=(e,t,i)=>(i.configurable=!0,i.enumerable=!0,Reflect.decorate&&typeof t!="object"&&Object.defineProperty(e,t,i),i);/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */function zo(e,t){return(i,r,o)=>{const n=s=>{var a;return((a=s.renderRoot)==null?void 0:a.querySelector(e))??null};return Bp(i,r,{get(){return n(this)}})}}const _n=50,pd={type:"heading",heading:"新建部件",heading_style:"title",badges:[{type:"button",entity:"",icon:"mdi:gesture-tap-button",text:"触发",value:!0,show_name:!0}]};function fd(e){if(e!==void 0)return typeof e=="boolean"?e?{}:void 0:e}function gd(e=!0){return{type:"grid",cards:e?[{...pd}]:[]}}function Ul(e){return{title:(e==null?void 0:e.title)??"新建视图",path:(e==null?void 0:e.path)??"view",icon:(e==null?void 0:e.icon)??"mdi:view-dashboard",max_columns:4,sections:[gd(!0)]}}function Fp(e){if(e.type==="button")return{...e,type:"trigger"};if(e.type==="color")return{...e,type:"rgba"};if(e.type==="bar"&&e.entity&&!e.entities){const{entity:t,name:i,...r}=e;return{...r,type:"bar",entities:[{entity:String(t),name:i?String(i):void 0}]}}return e}function Hp(e){const{title:t,icon:i,badges:r,...o}=e,n=[...o.cards??[]].map(l=>Fp(l)),s=!!(t||i||r!=null&&r.length),a=n.some(l=>l.type==="heading");return s&&!a&&n.unshift({type:"heading",heading:t||"新建部件",icon:i,badges:r==null?void 0:r.map(l=>({type:l.type??"entity",entity:l.entity,name:l.name,icon:l.icon}))}),{type:"grid",column_span:o.column_span,row_span:o.row_span,background:o.background,theme:o.theme,cards:n}}function md(e){return{...e,views:e.views.map(t=>Up(t))}}function Up(e){return{...e,sections:e.sections.map(t=>Hp(t))}}const Sa={"mdi:home":sd,"mdi:hand-wave":np,"mdi:sofa":op,"mdi:thermometer":rp,"mdi:water-percent":ip,"mdi:floor-lamp":tp,"mdi:spotlight-beam":ep,"mdi:window-shutter":Ju,"mdi:air-conditioner":Qu,"mdi:silverware-fork-knife":Zu,"mdi:coffee":Ku,"mdi:fridge":Wu,"mdi:dishwasher":Yu,"mdi:lightning-bolt":Xu,"mdi:flash":Gu,"mdi:floor-plan":qu,"mdi:sine-wave":Vu,"mdi:molecule-co2":ju,"mdi:view-dashboard":Uu,"mdi:cog":nd,"mdi:menu":_a,"mdi:menu-open":od,"mdi:bell":Hu,"mdi:cast-audio":Fu,"mdi:clock-outline":Bu,"mdi:home-thermometer":Lu,"mdi:palette":Ru,"mdi:axis-arrow":Nu,"mdi:link-variant":zu,"mdi:play":Mu,"mdi:stop":Iu,"mdi:pencil":ba,"mdi:undo":rd,"mdi:redo":id,"mdi:lightbulb":Du,"mdi:help-circle":vn,"mdi:eye":Au,"mdi:alert-circle":Ou,"mdi:toggle-switch":Tu,"mdi:gesture-tap-button":Pu,"mdi:brightness-6":Eu,"mdi:format-title":Cu,"mdi:view-grid":Su,"mdi:gauge":ku,"mdi:tune-vertical":$u,"mdi:volume-high":wu,"mdi:volume-off":xu,"mdi:text":yu,"mdi:dots-vertical":_u,"mdi:content-copy":bu,"mdi:content-cut":vu,"mdi:content-paste":mu,"mdi:plus-circle-multiple-outline":gu,"mdi:delete":fu,"mdi:cursor-move":pu,"mdi:magnify":td};function jp(e){let t=e.replace(/^mdi/,"");return t=t.replace(/([a-z])([A-Z0-9])/g,"$1-$2"),t=t.replace(/([0-9])([A-Z])/g,"$1-$2"),`mdi:${t.toLowerCase()}`}let er=null,yn=null,Cs=null;const On=new Set;function vd(e){return er?(e(),()=>{}):(On.add(e),()=>On.delete(e))}async function go(){er||(Cs||(Cs=Wi(()=>import("./mdi-icons-CYzTwnk_.js"),[],import.meta.url).then(e=>{er=new Map(Object.entries(Sa));const t=[];for(const[i,r]of Object.entries(e)){if(!i.startsWith("mdi")||typeof r!="string")continue;const o=jp(i);er.set(o,r),t.push(o)}yn=t.sort();for(const i of On)i();On.clear()})),await Cs)}function mi(e){return e?er?er.get(e)??vn:(go(),Sa[e]??vn):vn}function Jo(e,t=80){const i=e.trim().toLowerCase();if(!yn){const o=Object.keys(Sa);return i?o.filter(n=>n.includes(i)).slice(0,t):o.slice(0,t)}if(!i)return yn.slice(0,t);const r=[];for(const o of yn)if(o.includes(i)&&(r.push(o),r.length>=t))break;return r}var Vp=Object.defineProperty,qp=Object.getOwnPropertyDescriptor,No=(e,t,i,r)=>{for(var o=r>1?void 0:r?qp(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Vp(t,i,o),o};let An=class extends y{constructor(){super(...arguments),this.path=""}render(){return u`
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d=${this.path}></path>
      </svg>
    `}};An.styles=$`
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
  `;No([p({attribute:!1})],An.prototype,"path",2);An=No([w("ha-svg-icon")],An);let mo=class extends y{constructor(){super(...arguments),this.icon="",this._revision=0}connectedCallback(){super.connectedCallback(),go(),this._unload=vd(()=>{this._revision++})}disconnectedCallback(){var e;(e=this._unload)==null||e.call(this),super.disconnectedCallback()}render(){this._revision;const e=mi(this.icon);return u`<ha-svg-icon .path=${e}></ha-svg-icon>`}};mo.styles=$`
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
  `;No([p()],mo.prototype,"icon",2);No([b()],mo.prototype,"_revision",2);mo=No([w("ha-icon")],mo);var Gp=Object.getOwnPropertyDescriptor,Xp=(e,t,i,r)=>{for(var o=r>1?void 0:r?Gp(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=s(o)||o);return o};let Xs=class extends y{render(){return u`<slot></slot>`}};Xs.styles=$`
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
  `;Xs=Xp([w("ha-card")],Xs);/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */let bd=class extends Event{constructor(t,i,r,o){super("context-request",{bubbles:!0,composed:!0}),this.context=t,this.contextTarget=i,this.callback=r,this.subscribe=o??!1}};/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 *//**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */let jl=class{constructor(t,i,r,o){if(this.subscribe=!1,this.provided=!1,this.value=void 0,this.t=(n,s)=>{this.unsubscribe&&(this.unsubscribe!==s&&(this.provided=!1,this.unsubscribe()),this.subscribe||this.unsubscribe()),this.value=n,this.host.requestUpdate(),this.provided&&!this.subscribe||(this.provided=!0,this.callback&&this.callback(n,s)),this.unsubscribe=s},this.host=t,i.context!==void 0){const n=i;this.context=n.context,this.callback=n.callback,this.subscribe=n.subscribe??!1}else this.context=i,this.callback=r,this.subscribe=o??!1;this.host.addController(this)}hostConnected(){this.dispatchRequest()}hostDisconnected(){this.unsubscribe&&(this.unsubscribe(),this.unsubscribe=void 0)}dispatchRequest(){this.host.dispatchEvent(new bd(this.context,this.host,this.t,this.subscribe))}};/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */let Yp=class{get value(){return this.o}set value(t){this.setValue(t)}setValue(t,i=!1){const r=i||!Object.is(t,this.o);this.o=t,r&&this.updateObservers()}constructor(t){this.subscriptions=new Map,this.updateObservers=()=>{for(const[i,{disposer:r}]of this.subscriptions)i(this.o,r)},t!==void 0&&(this.value=t)}addCallback(t,i,r){if(!r)return void t(this.value);this.subscriptions.has(t)||this.subscriptions.set(t,{disposer:()=>{this.subscriptions.delete(t)},consumerHost:i});const{disposer:o}=this.subscriptions.get(t);t(this.value,o)}clearCallbacks(){this.subscriptions.clear()}};/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */let Wp=class extends Event{constructor(t,i){super("context-provider",{bubbles:!0,composed:!0}),this.context=t,this.contextTarget=i}},Vl=class extends Yp{constructor(t,i,r){var o,n;super(i.context!==void 0?i.initialValue:r),this.onContextRequest=s=>{if(s.context!==this.context)return;const a=s.contextTarget??s.composedPath()[0];a!==this.host&&(s.stopPropagation(),this.addCallback(s.callback,a,s.subscribe))},this.onProviderRequest=s=>{if(s.context!==this.context||(s.contextTarget??s.composedPath()[0])===this.host)return;const a=new Set;for(const[l,{consumerHost:c}]of this.subscriptions)a.has(l)||(a.add(l),c.dispatchEvent(new bd(this.context,c,l,!0)));s.stopPropagation()},this.host=t,i.context!==void 0?this.context=i.context:this.context=i,this.attachListeners(),(n=(o=this.host).addController)==null||n.call(o,this)}attachListeners(){this.host.addEventListener("context-request",this.onContextRequest),this.host.addEventListener("context-provider",this.onProviderRequest)}hostConnected(){this.host.dispatchEvent(new Wp(this.context,this.host))}};/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */function Kp({context:e}){return(t,i)=>{const r=new WeakMap;if(typeof i=="object")return{get(){return t.get.call(this)},set(o){return r.get(this).setValue(o),t.set.call(this,o)},init(o){return r.set(this,new Vl(this,{context:e,initialValue:o})),o}};{t.constructor.addInitializer(s=>{r.set(s,new Vl(s,{context:e}))});const o=Object.getOwnPropertyDescriptor(t,i);let n;if(o===void 0){const s=new WeakMap;n={get(){return s.get(this)},set(a){r.get(this).setValue(a),s.set(this,a)},configurable:!0,enumerable:!0}}else{const s=o.set;n={...o,set(a){r.get(this).setValue(a),s==null||s.call(this,a)}}}return void Object.defineProperty(t,i,n)}}}/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */function Tr({context:e,subscribe:t}){return(i,r)=>{typeof r=="object"?r.addInitializer(function(){new jl(this,{context:e,callback:o=>{i.set.call(this,o)},subscribe:t})}):i.constructor.addInitializer(o=>{new jl(o,{context:e,callback:n=>{o[r]=n},subscribe:t})})}}/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const Ca={ATTRIBUTE:1,CHILD:2},Ea=e=>(...t)=>({_$litDirective$:e,values:t});let Pa=class{constructor(t){}get _$AU(){return this._$AM._$AU}_$AT(t,i,r){this._$Ct=t,this._$AM=i,this._$Ci=r}_$AS(t,i){return this.update(t,i)}update(t,i){return this.render(...i)}};/**
 * @license
 * Copyright 2018 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const _d="important",Zp=" !"+_d,T=Ea(class extends Pa{constructor(e){var t;if(super(e),e.type!==Ca.ATTRIBUTE||e.name!=="style"||((t=e.strings)==null?void 0:t.length)>2)throw Error("The `styleMap` directive must be used in the `style` attribute and must be the only part in the attribute.")}render(e){return Object.keys(e).reduce((t,i)=>{const r=e[i];return r==null?t:t+`${i=i.includes("-")?i:i.replace(/(?:^(webkit|moz|ms|o)|)(?=[A-Z])/g,"-$&").toLowerCase()}:${r};`},"")}update(e,[t]){const{style:i}=e.element;if(this.ft===void 0)return this.ft=new Set(Object.keys(t)),this.render(t);for(const r of this.ft)t[r]==null&&(this.ft.delete(r),r.includes("-")?i.removeProperty(r):i[r]=null);for(const r in t){const o=t[r];if(o!=null){this.ft.add(r);const n=typeof o=="string"&&o.endsWith(Zp);r.includes("-")||n?i.setProperty(r,n?o.slice(0,-11):o,n?_d:""):i[r]=o}}return Et}}),zi="flow";function Re(e){if(!e)return!1;const t=e.state;if(typeof t=="boolean")return t;if(typeof t=="number")return t>0;const i=String(t).toLowerCase();return i==="on"||i==="true"||i==="1"||i==="open"||i==="playing"}function yd(e){const t=Re(e);return typeof(e==null?void 0:e.state)=="number"?t?0:100:!t}const io=[{value:"primary",label:"主题色"},{value:"accent",label:"强调色"},{value:"red",label:"红色"},{value:"pink",label:"粉色"},{value:"purple",label:"紫色"},{value:"deep-purple",label:"深紫"},{value:"indigo",label:"靛蓝"},{value:"blue",label:"蓝色"},{value:"light-blue",label:"浅蓝"},{value:"cyan",label:"青色"},{value:"teal",label:"蓝绿"},{value:"green",label:"绿色"},{value:"light-green",label:"浅绿"},{value:"lime",label:"青柠"},{value:"yellow",label:"黄色"},{value:"amber",label:"琥珀色"},{value:"orange",label:"橙色"},{value:"deep-orange",label:"深橙"},{value:"brown",label:"棕色"},{value:"grey",label:"灰色"},{value:"blue-grey",label:"蓝灰"}],Qp=new Set(io.map(e=>e.value));function Xt(e){return Qp.has(e)?`var(--${e}-color, var(--primary-color))`:e}function Jp(e){var t;return((t=io.find(i=>i.value===e))==null?void 0:t.label)??e}function xd(e){return(e==null?void 0:e.show_icon)!==!1}function Yt(e){return e?typeof e.show_name=="boolean"?e.show_name:typeof e.show_state=="boolean"?e.show_state:!0:!0}var ef=Object.defineProperty,tf=Object.getOwnPropertyDescriptor,wd=(e,t,i,r)=>{for(var o=r>1?void 0:r?tf(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ef(t,i,o),o};let Dn=class extends y{constructor(){super(...arguments),this.type="text"}render(){return u`
      <div
        class="heading-badge"
        role=${this.type==="button"?"button":"group"}
        tabindex=${this.type==="button"?"0":"-1"}
      >
        <slot name="icon"></slot>
        <slot></slot>
      </div>
    `}};Dn.styles=$`
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
  `;wd([p()],Dn.prototype,"type",2);Dn=wd([w("ha-heading-badge")],Dn);var rf=Object.defineProperty,of=Object.getOwnPropertyDescriptor,Ro=(e,t,i,r)=>{for(var o=r>1?void 0:r?of(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&rf(t,i,o),o};let yi=class extends y{constructor(){super(...arguments),this.statesRevision=0,this._tick=0}connectedCallback(){super.connectedCallback(),this._syncSubscription()}disconnectedCallback(){var e;(e=this._unsub)==null||e.call(this),this._unsub=void 0,this._subscribedEntity=void 0,super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("config"))&&this._syncSubscription()}_entityId(){var e;return(e=this.config)!=null&&e.entity?String(this.config.entity).trim():""}_syncSubscription(){var t;const e=this._entityId();e===this._subscribedEntity&&this._unsub||((t=this._unsub)==null||t.call(this),this._unsub=void 0,this._subscribedEntity=void 0,!(!e||!this.flow)&&(this._subscribedEntity=e,this._unsub=this.flow.subscribeEntity(e,()=>{this._tick+=1})))}_stateText(){const e=this._entityId();return!e||!this.flow?"—":this.flow.formatState(e)}_iconColor(){var o;const e=(o=this.config)!=null&&o.color?String(this.config.color).trim():"";if(!e||e==="none")return;const t=this._entityId(),i=t&&this.flow?this.flow.states[t]:void 0,r=Re(i);return e==="state"?r?"var(--state-icon-color)":void 0:r?Xt(e):void 0}render(){var l;const e=this.config;if(!e)return m;this._tick,this.statesRevision;const t=this._entityId(),i=t&&this.flow?this.flow.states[t]:void 0,r=xd(e),o=Yt(e),n=!t,s=n?"mdi:alert-circle":e.icon?String(e.icon):(l=i==null?void 0:i.attributes)!=null&&l.icon?String(i.attributes.icon):"mdi:eye",a=n?"var(--error-color)":this._iconColor();return u`
      <ha-heading-badge
        class=${n?"error":""}
        style=${T({"--icon-color":a})}
      >
        ${r?u`<ha-icon slot="icon" .icon=${s}></ha-icon>`:m}
        ${o?u`<span>${n?"—":this._stateText()}</span>`:m}
      </ha-heading-badge>
    `}};yi.styles=$`
    :host {
      display: inline-flex;
    }
    ha-heading-badge.error {
      --icon-color: var(--error-color, #b91c1c);
    }
  `;Ro([Tr({context:zi,subscribe:!0}),p({attribute:!1})],yi.prototype,"flow",2);Ro([p({attribute:!1})],yi.prototype,"config",2);Ro([p({type:Number,attribute:!1})],yi.prototype,"statesRevision",2);Ro([b()],yi.prototype,"_tick",2);yi=Ro([w("hui-entity-heading-badge")],yi);/**
 * @license
 * Copyright 2018 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const O=Ea(class extends Pa{constructor(e){var t;if(super(e),e.type!==Ca.ATTRIBUTE||e.name!=="class"||((t=e.strings)==null?void 0:t.length)>2)throw Error("`classMap()` can only be used in the `class` attribute and must be the only part in the attribute.")}render(e){return" "+Object.keys(e).filter(t=>e[t]).join(" ")+" "}update(e,[t]){var r,o;if(this.st===void 0){this.st=new Set,e.strings!==void 0&&(this.nt=new Set(e.strings.join(" ").split(/\s/).filter(n=>n!=="")));for(const n in t)t[n]&&!((r=this.nt)!=null&&r.has(n))&&this.st.add(n);return this.render(t)}const i=e.element.classList;for(const n of this.st)n in t||(i.remove(n),this.st.delete(n));for(const n in t){const s=!!t[n];s===this.st.has(n)||(o=this.nt)!=null&&o.has(n)||(s?(i.add(n),this.st.add(n)):(i.remove(n),this.st.delete(n)))}return Et}});function Or(e,t){if(!e)return;const i=t?String(t).trim():"";if(i)return Xt(i)}function Ni(e,t,i){const r=Or(e,t);if(r)return r;if(e)return i?Xt(i):void 0}function nf(e,t){return Ni(e,t,"amber")}function sf(e,t){return Or(e,t)}function Ta(e){const t=e?String(e).trim():"";if(t)return Xt(t)}function rs(e){if(typeof e=="boolean"||typeof e=="number")return e;if(e==null)return!0;const t=String(e).trim();if(t===""||t==="true")return!0;if(t==="false")return!1;const i=Number(t);return t!==""&&Number.isFinite(i)&&String(i)===t?i:t}var af=Object.defineProperty,lf=Object.getOwnPropertyDescriptor,os=(e,t,i,r)=>{for(var o=r>1?void 0:r?lf(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&af(t,i,o),o};let or=class extends y{constructor(){super(...arguments),this._flashing=!1}disconnectedCallback(){this._pulseTimer&&clearTimeout(this._pulseTimer),super.disconnectedCallback()}_pulse(){this._flashing=!0,this._pulseTimer&&clearTimeout(this._pulseTimer),this._pulseTimer=setTimeout(()=>{this._flashing=!1,this._pulseTimer=void 0},700)}async _tap(e){var r,o;e.stopPropagation();const t=(r=this.config)!=null&&r.entity?String(this.config.entity):"";if(!t||!this.flow)return;const i=rs((o=this.config)==null?void 0:o.value);await this.flow.callService(t,i),this._pulse()}render(){const e=this.config;if(!e)return m;const t=e.icon?String(e.icon):"",i=e.text?String(e.text):e.name?String(e.name):"",r=Yt(e)&&!!i,o=e.entity?String(e.entity).trim():"",n=o&&this.flow?this.flow.states[o]:void 0,s=this._flashing||Re(n),a=Or(s,e.color),l=!!e.entity;return u`
      <button
        type="button"
        class=${O({badge:!0,"with-text":r,colored:!!a,flashing:this._flashing&&!a})}
        style=${T({"--color":a})}
        ?disabled=${!l}
        @click=${this._tap}
      >
        ${t?u`<ha-icon .icon=${t}></ha-icon>`:m}
        ${r?u`<span class="text">${i}</span>`:m}
      </button>
    `}};or.styles=$`
    :host {
      display: inline-flex;
    }
    .badge {
      display: inline-flex;
      flex-direction: row;
      align-items: center;
      justify-content: center;
      gap: 2px;
      height: 26px;
      min-width: 26px;
      padding: 0;
      border: none;
      border-radius: 999px;
      background: color-mix(in srgb, var(--primary-text-color) 8%, transparent);
      color: var(--secondary-text-color);
      cursor: pointer;
      font: inherit;
      font-size: 12px;
      font-weight: 500;
      line-height: 1;
      white-space: nowrap;
    }
    .badge.with-text {
      padding: 0 8px;
    }
    .badge.colored {
      color: var(--color);
      background: color-mix(in srgb, var(--color) 20%, transparent);
    }
    .badge.flashing {
      color: var(--state-icon-color, var(--primary-color));
      background: color-mix(in srgb, var(--state-icon-color, var(--primary-color)) 20%, transparent);
    }
    .badge:disabled {
      cursor: default;
      opacity: 0.55;
    }
    .badge:not(:disabled):hover {
      filter: brightness(0.96);
    }
    ha-icon {
      --mdc-icon-size: 16px;
    }
    .text {
      padding: 0 4px;
    }
  `;os([p({attribute:!1})],or.prototype,"flow",2);os([p({attribute:!1})],or.prototype,"config",2);os([b()],or.prototype,"_flashing",2);or=os([w("hui-button-heading-badge")],or);var cf=Object.defineProperty,df=Object.getOwnPropertyDescriptor,Oa=(e,t,i,r)=>{for(var o=r>1?void 0:r?df(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&cf(t,i,o),o};let nr=class extends y{constructor(){super(...arguments),this._badgeUnsubs=[],this._subscribedKey=""}static getStubConfig(){return{...pd}}setConfig(e){this._config=e,this._syncBadgeSubscriptions()}connectedCallback(){super.connectedCallback(),this._syncBadgeSubscriptions()}disconnectedCallback(){this._clearBadgeSubscriptions(),super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("_config"))&&this._syncBadgeSubscriptions()}_clearBadgeSubscriptions(){this._badgeUnsubs.forEach(e=>e()),this._badgeUnsubs=[],this._subscribedKey=""}_syncBadgeSubscriptions(){var r;const e=this.flow,t=((r=this._config)==null?void 0:r.badges)??[],i=`${e?"1":"0"}:${t.map(o=>String(o.entity??"").trim()).join("\0")}`;i===this._subscribedKey&&this._badgeUnsubs.length>0||(this._clearBadgeSubscriptions(),e&&(this._subscribedKey=i,t.forEach(o=>{const n=o.entity?String(o.entity).trim():"";n&&this._badgeUnsubs.push(e.subscribeEntity(n,()=>{this.requestUpdate()}))})))}getGridOptions(){return{columns:"full",rows:"auto",min_columns:3}}_renderBadge(e){var i;return String(e.type??"entity")==="button"?u`
        <hui-button-heading-badge .flow=${this.flow} .config=${e}></hui-button-heading-badge>
      `:u`
        <hui-entity-heading-badge
          .flow=${this.flow}
          .config=${e}
          .statesRevision=${((i=this.flow)==null?void 0:i.statesRevision)??0}
        ></hui-entity-heading-badge>
    `}render(){var o;if(!this._config)return m;(o=this.flow)==null||o.statesRevision;const e=String(this._config.heading??""),t=this._config.icon?String(this._config.icon):void 0,i=String(this._config.heading_style??"title"),r=this._config.badges??[];return u`
      <ha-card>
        <div class="container">
          <div class="content ${i}">
            ${t?u`<ha-icon .icon=${t}></ha-icon>`:m}
            ${e?u`<p>${e}</p>`:m}
          </div>
          ${r.length?u`
                <div class="badges">
                  <div class="badges-row">
                    ${r.map(n=>this._renderBadge(n))}
                  </div>
                </div>
              `:m}
        </div>
      </ha-card>
    `}};nr.styles=$`
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
  `;Oa([p({attribute:!1})],nr.prototype,"flow",2);Oa([b()],nr.prototype,"_config",2);nr=Oa([w("hui-heading-card")],nr);var hf=Object.defineProperty,uf=Object.getOwnPropertyDescriptor,$d=(e,t,i,r)=>{for(var o=r>1?void 0:r?uf(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&hf(t,i,o),o};let vo=class extends y{setConfig(e){this._config=e}getGridOptions(){return{columns:"full",rows:2}}render(){var t;const e=String(((t=this._config)==null?void 0:t.message)??"Unknown error");return u`
      <ha-card>
        <div class="error">${e}</div>
      </ha-card>
    `}};vo.styles=$`
    .error {
      padding: 16px;
      color: var(--error-color, #b91c1c);
      font-size: 14px;
    }
  `;$d([p({attribute:!1})],vo.prototype,"flow",2);vo=$d([w("hui-error-card")],vo);function Aa(){return{async:!1,breaks:!1,extensions:null,gfm:!0,hooks:null,pedantic:!1,renderer:null,silent:!1,tokenizer:null,walkTokens:null}}var Ri=Aa();function kd(e){Ri=e}var ui={exec:()=>null};function ji(e){let t=[];return i=>{let r=Math.max(0,Math.min(3,i-1)),o=t[r];return o||(o=e(r),t[r]=o),o}}function A(e,t=""){let i=typeof e=="string"?e:e.source,r={replace:(o,n)=>{let s=typeof n=="string"?n:n.source;return s=s.replace(se.caret,"$1"),i=i.replace(o,s),r},getRegex:()=>new RegExp(i,t)};return r}var pf=((e="")=>{try{return!!new RegExp("(?<=1)(?<!1)"+e)}catch{return!1}})(),se={codeRemoveIndent:/^(?: {1,4}| {0,3}\t)/gm,outputLinkReplace:/\\([\[\]])/g,indentCodeCompensation:/^(\s+)(?:```)/,beginningSpace:/^\s+/,endingHash:/#$/,startingSpaceChar:/^ /,endingSpaceChar:/ $/,nonSpaceChar:/[^ ]/,newLineCharGlobal:/\n/g,tabCharGlobal:/\t/g,multipleSpaceGlobal:/\s+/g,blankLine:/^[ \t]*$/,doubleBlankLine:/\n[ \t]*\n[ \t]*$/,blockquoteStart:/^ {0,3}>/,blockquoteSetextReplace:/\n {0,3}((?:=+|-+) *)(?=\n|$)/g,blockquoteSetextReplace2:/^ {0,3}>[ \t]?/gm,listReplaceNesting:/^ {1,4}(?=( {4})*[^ ])/g,listIsTask:/^\[[ xX]\] +\S/,listReplaceTask:/^\[[ xX]\] +/,listTaskCheckbox:/\[[ xX]\]/,anyLine:/\n.*\n/,hrefBrackets:/^<(.*)>$/,tableDelimiter:/[:|]/,tableAlignChars:/^\||\| *$/g,tableRowBlankLine:/\n[ \t]*$/,tableAlignRight:/^ *-+: *$/,tableAlignCenter:/^ *:-+: *$/,tableAlignLeft:/^ *:-+ *$/,startATag:/^<a /i,endATag:/^<\/a>/i,startPreScriptTag:/^<(pre|code|kbd|script)(\s|>)/i,endPreScriptTag:/^<\/(pre|code|kbd|script)(\s|>)/i,startAngleBracket:/^</,endAngleBracket:/>$/,pedanticHrefTitle:/^([^'"]*[^\s])\s+(['"])(.*)\2/,unicodeAlphaNumeric:/[\p{L}\p{N}]/u,escapeTest:/[&<>"']/,escapeReplace:/[&<>"']/g,escapeTestNoEncode:/[<>"']|&(?!(#\d{1,7}|#[Xx][a-fA-F0-9]{1,6}|\w+);)/,escapeReplaceNoEncode:/[<>"']|&(?!(#\d{1,7}|#[Xx][a-fA-F0-9]{1,6}|\w+);)/g,caret:/(^|[^\[])\^/g,percentDecode:/%25/g,findPipe:/\|/g,splitPipe:/ \|/,slashPipe:/\\\|/g,carriageReturn:/\r\n|\r/g,spaceLine:/^ +$/gm,notSpaceStart:/^\S*/,endingNewline:/\n$/,listItemRegex:e=>new RegExp(`^( {0,3}${e})((?:[	 ][^\\n]*)?(?:\\n|$))`),nextBulletRegex:ji(e=>new RegExp(`^ {0,${e}}(?:[*+-]|\\d{1,9}[.)])((?:[ 	][^\\n]*)?(?:\\n|$))`)),hrRegex:ji(e=>new RegExp(`^ {0,${e}}((?:- *){3,}|(?:_ *){3,}|(?:\\* *){3,})(?:\\n+|$)`)),fencesBeginRegex:ji(e=>new RegExp(`^ {0,${e}}(?:\`\`\`|~~~)`)),headingBeginRegex:ji(e=>new RegExp(`^ {0,${e}}#`)),htmlBeginRegex:ji(e=>new RegExp(`^ {0,${e}}<(?:[a-z].*>|!--)`,"i")),blockquoteBeginRegex:ji(e=>new RegExp(`^ {0,${e}}>`))},ff=/^(?:[ \t]*(?:\n|$))+/,gf=/^((?: {4}| {0,3}\t)[^\n]+(?:\n(?:[ \t]*(?:\n|$))*)?)+/,mf=/^ {0,3}(`{3,}(?=[^`\n]*(?:\n|$))|~{3,})([^\n]*)(?:\n|$)(?:|([\s\S]*?)(?:\n|$))(?: {0,3}\1[~`]* *(?=\n|$)|$)/,Lo=/^ {0,3}((?:-[\t ]*){3,}|(?:_[ \t]*){3,}|(?:\*[ \t]*){3,})(?:\n+|$)/,vf=/^ {0,3}(#{1,6})(?=\s|$)(.*)(?:\n+|$)/,Da=/ {0,3}(?:[*+-]|\d{1,9}[.)])/,Sd=/^(?!bull |blockCode|fences|blockquote|heading|html|table)((?:.|\n(?!\s*?\n|bull |blockCode|fences|blockquote|heading|html|table))+?)\n {0,3}(=+|-+) *(?:\n+|$)/,Cd=A(Sd).replace(/bull/g,Da).replace(/blockCode/g,/(?: {4}| {0,3}\t)/).replace(/fences/g,/ {0,3}(?:`{3,}|~{3,})/).replace(/blockquote/g,/ {0,3}>/).replace(/heading/g,/ {0,3}#{1,6}(?:\s|$)/).replace(/html/g,/ {0,3}<[^\n>]+>\n/).replace(/\|table/g,"").getRegex(),bf=A(Sd).replace(/bull/g,Da).replace(/blockCode/g,/(?: {4}| {0,3}\t)/).replace(/fences/g,/ {0,3}(?:`{3,}|~{3,})/).replace(/blockquote/g,/ {0,3}>/).replace(/heading/g,/ {0,3}#{1,6}(?:\s|$)/).replace(/html/g,/ {0,3}<[^\n>]+>\n/).replace(/table/g,/ {0,3}\|?(?:[:\- ]*\|)+[\:\- ]*\n/).getRegex(),Ia=/^([^\n]+(?:\n(?!hr|heading|lheading|blockquote|fences|list|html|table|[ \t]+\n)[^\n]+)*)/,_f=/^[^\n]+/,Ma=/(?!\s*\])(?:\\[\s\S]|[^\[\]\\])+/,yf=A(/^ {0,3}\[(label)\]: *(?:\n[ \t]*)?([^<\s][^\s]*|<.*?>)(?:(?: +(?:\n[ \t]*)?| *\n[ \t]*)(title))? *(?:\n+|$)/).replace("label",Ma).replace("title",/(?:"(?:\\"?|[^"\\])*"|'[^'\n]*(?:\n[^'\n]+)*\n?'|\([^()]*\))/).getRegex(),xf=A(/^(bull)([ \t][^\n]*?)?(?:\n|$)/).replace(/bull/g,Da).getRegex(),ns="address|article|aside|base|basefont|blockquote|body|caption|center|col|colgroup|dd|details|dialog|dir|div|dl|dt|fieldset|figcaption|figure|footer|form|frame|frameset|h[1-6]|head|header|hr|html|iframe|legend|li|link|main|menu|menuitem|meta|nav|noframes|ol|optgroup|option|p|param|search|section|summary|table|tbody|td|tfoot|th|thead|title|tr|track|ul",za=/<!--(?:-?>|[\s\S]*?(?:-->|$))/,wf=A("^ {0,3}(?:<(script|pre|style|textarea)[\\s>][\\s\\S]*?(?:</\\1>[^\\n]*\\n*|$)|comment[^\\n]*(\\n+|$)|<\\?[\\s\\S]*?(?:\\?>[^\\n]*\\n*|$)|<![A-Z][\\s\\S]*?(?:>[^\\n]*\\n*|$)|<!\\[CDATA\\[[\\s\\S]*?(?:\\]\\]>[^\\n]*\\n*|$)|</?(tag)(?: +|\\n|/?>)[\\s\\S]*?(?:(?:\\n[ 	]*)+\\n|$)|<(?!script|pre|style|textarea)([a-z][\\w-]*)(?:attribute)*? */?>(?=[ \\t]*(?:\\n|$))[\\s\\S]*?(?:(?:\\n[ 	]*)+\\n|$)|</(?!script|pre|style|textarea)[a-z][\\w-]*\\s*>(?=[ \\t]*(?:\\n|$))[\\s\\S]*?(?:(?:\\n[ 	]*)+\\n|$))","i").replace("comment",za).replace("tag",ns).replace("attribute",/ +[a-zA-Z:_][\w.:-]*(?: *= *"[^"\n]*"| *= *'[^'\n]*'| *= *[^\s"'=<>`]+)?/).getRegex(),Ed=e=>A(Ia).replace("hr",Lo).replace("heading"," {0,3}#{1,6}(?:\\s|$)").replace("|lheading","").replace("|table","").replace("blockquote"," {0,3}>").replace("fences"," {0,3}(?:`{3,}(?=[^`\\n]*(?:\\n|$))|~~~)[^\\n]*(?:\\n|$)").replace("list",e).replace("html","</?(?:tag)(?: +|\\n|/?>)|<(?:script|pre|style|textarea|!--)").replace("tag",ns).getRegex(),$f=Ed(/ {0,3}(?:[*+-]|1[.)])[ \t]+[^ \t\n]/),kf=Ed(/ {0,3}(?:[*+-]|\d{1,9}[.)])(?:[ \t]|\n|$)/),Sf=A(/^( {0,3}> ?(paragraph|[^\n]*)(?:\n|$))+/).replace("paragraph",kf).getRegex(),Na={blockquote:Sf,code:gf,def:yf,fences:mf,heading:vf,hr:Lo,html:wf,lheading:Cd,list:xf,newline:ff,paragraph:$f,table:ui,text:_f},ql=A("^ *([^\\n ].*)\\n {0,3}((?:\\| *)?:?-+:? *(?:\\| *:?-+:? *)*(?:\\| *)?)(?:\\n((?:(?! *\\n|hr|heading|blockquote|code|fences|list|html).*(?:\\n|$))*)\\n*|$)").replace("hr",Lo).replace("heading"," {0,3}#{1,6}(?:\\s|$)").replace("blockquote"," {0,3}>").replace("code","(?: {4}| {0,3}	)[^\\n]").replace("fences"," {0,3}(?:`{3,}(?=[^`\\n]*(?:\\n|$))|~~~)[^\\n]*(?:\\n|$)").replace("list"," {0,3}(?:[*+-]|1[.)])[ \\t]").replace("html","</?(?:tag)(?: +|\\n|/?>)|<(?:script|pre|style|textarea|!--)").replace("tag",ns).getRegex(),Cf={...Na,lheading:bf,table:ql,paragraph:A(Ia).replace("hr",Lo).replace("heading"," {0,3}#{1,6}(?:\\s|$)").replace("|lheading","").replace("table",ql).replace("blockquote"," {0,3}>").replace("fences"," {0,3}(?:`{3,}(?=[^`\\n]*(?:\\n|$))|~~~)[^\\n]*(?:\\n|$)").replace("list"," {0,3}(?:[*+-]|1[.)])[ \\t]+[^ \\t\\n]").replace("html","</?(?:tag)(?: +|\\n|/?>)|<(?:script|pre|style|textarea|!--)").replace("tag",ns).getRegex()},Ef={...Na,html:A(`^ *(?:comment *(?:\\n|\\s*$)|<(tag)[\\s\\S]+?</\\1> *(?:\\n{2,}|\\s*$)|<tag(?:"[^"]*"|'[^']*'|\\s[^'"/>\\s]*)*?/?> *(?:\\n{2,}|\\s*$))`).replace("comment",za).replace(/tag/g,"(?!(?:a|em|strong|small|s|cite|q|dfn|abbr|data|time|code|var|samp|kbd|sub|sup|i|b|u|mark|ruby|rt|rp|bdi|bdo|span|br|wbr|ins|del|img)\\b)\\w+(?!:|[^\\w\\s@]*@)\\b").getRegex(),def:/^ *\[([^\]]+)\]: *<?([^\s>]+)>?(?: +(["(][^\n]+[")]))? *(?:\n+|$)/,heading:/^(#{1,6})(.*)(?:\n+|$)/,fences:ui,lheading:/^(.+?)\n {0,3}(=+|-+) *(?:\n+|$)/,paragraph:A(Ia).replace("hr",Lo).replace("heading",` *#{1,6} *[^
]`).replace("lheading",Cd).replace("|table","").replace("blockquote"," {0,3}>").replace("|fences","").replace("|list","").replace("|html","").replace("|tag","").getRegex()},Pf=/^\\([!"#$%&'()*+,\-./:;<=>?@\[\]\\^_`{|}~])/,Tf=/^(`+)([^`]|[^`][\s\S]*?[^`])\1(?!`)/,Pd=/^( {2,}|\\)\n(?!\s*$)/,Of=/^(`+|[^`])(?:(?= {2,}\n)|[\s\S]*?(?:(?=[\\<!\[`*_]|\b_|$)|[^ ](?= {2,}\n)))/,It=/[\p{P}\p{S}]/u,Ar=/[\s\p{P}\p{S}]/u,Bo=/[^\s\p{P}\p{S}]/u,Af=A(/^((?![*_])punctSpace)/,"u").replace(/punctSpace/g,Ar).getRegex(),Df=/[\p{Pi}\p{Ps}"']/u,Td=/(?!~)[\p{P}\p{S}]/u,If=/(?!~)[\s\p{P}\p{S}]/u,Mf=/(?:[^\s\p{P}\p{S}]|~)/u,zf=A(/link|precode-code|html/,"g").replace("link",/\[(?:[^\[\]`]|(?<a>`+)[^`]+\k<a>(?!`))*?\]\((?:\\[\s\S]|[^\\\(\)]|\((?:\\[\s\S]|[^\\\(\)])*\))*\)/).replace("precode-",pf?"(?<!`)()":"(^^|[^`])").replace("code",/(?<b>`+)[^`]+\k<b>(?!`)/).replace("html",/<(?! )[^<>]*?>/).getRegex(),Od=/^(?:\*+(?:((?!\*)punct)|([^\s*]))?)|^_+(?:((?!_)punct)|([^\s_]))?/,Nf=A(Od,"u").replace(/punct/g,It).getRegex(),Rf=A(Od,"u").replace(/punct/g,Td).getRegex(),Lf=/^(?:\*+(?:((?!\*)(?!openQuote)punct)|([^\s*]))?)|^_+(?:((?!_)(?!openQuote)punct)|([^\s_]))?/,Bf=A(Lf,"u").replace(/openQuote/g,Df).replace(/punct/g,It).getRegex(),Ad="^[^_*]*?__[^_*]*?\\*[^_*]*?(?=__)|[^*]+(?=[^*])|(?!\\*)punct(\\*+)(?=[\\s]|$)|notPunctSpace(\\*+)(?!\\*)(?=punctSpace|$)|(?!\\*)punctSpace(\\*+)(?=notPunctSpace)|[\\s](\\*+)(?!\\*)(?=punct)|(?!\\*)punct(\\*+)(?!\\*)(?=punct)|notPunctSpace(\\*+)(?=notPunctSpace)",Ff=A(Ad,"gu").replace(/notPunctSpace/g,Bo).replace(/punctSpace/g,Ar).replace(/punct/g,It).getRegex(),Hf=A(Ad,"gu").replace(/notPunctSpace/g,Mf).replace(/punctSpace/g,If).replace(/punct/g,Td).getRegex(),Uf="^[^_*]*?__[^_*]*?\\*[^_*]*?(?=__)|[^*]+(?=[^*])|(?!\\*)punct(\\*+)(?=[\\s]|$)|notPunctSpace(\\*+)(?!\\*)(?=punctSpace|$)|(?!\\*)[\\s](\\*+)(?=notPunctSpace)|[\\s](\\*+)(?!\\*)(?=punct)|(?!\\*)punct(\\*+)(?!\\*)(?=punct)|(?:(?!\\*)punct|notPunctSpace)(\\*+)(?!\\*)(?=notPunctSpace)",jf=A(Uf,"gu").replace(/notPunctSpace/g,Bo).replace(/punctSpace/g,Ar).replace(/punct/g,It).getRegex(),Vf=A("^[^_*]*?\\*\\*[^_*]*?_[^_*]*?(?=\\*\\*)|[^_]+(?=[^_])|(?!_)punct(_+)(?=[\\s]|$)|notPunctSpace(_+)(?!_)(?=punctSpace|$)|(?!_)punctSpace(_+)(?=notPunctSpace)|[\\s](_+)(?!_)(?=punct)|(?!_)punct(_+)(?!_)(?=punct)","gu").replace(/notPunctSpace/g,Bo).replace(/punctSpace/g,Ar).replace(/punct/g,It).getRegex(),qf="^[^_*]*?\\*\\*[^_*]*?_[^_*]*?(?=\\*\\*)|[^_]+(?=[^_])|(?!_)punct(_+)(?=[\\s]|$)|notPunctSpace(_+)(?!_)(?=punctSpace|$)|(?!_)[\\s](_+)(?=notPunctSpace)|[\\s](_+)(?!_)(?=punct)|(?!_)punct(_+)(?!_)(?=punct)|(?:(?!_)punct|notPunctSpace)(_+)(?!_)(?=notPunctSpace)",Gf=A(qf,"gu").replace(/notPunctSpace/g,Bo).replace(/punctSpace/g,Ar).replace(/punct/g,It).getRegex(),Xf=A(/^~~?(?:((?!~)punct)|[^\s~])/,"u").replace(/punct/g,It).getRegex(),Yf="^[^~]+(?=[^~])|(?!~)punct(~~?)(?=[\\s]|$)|notPunctSpace(~~?)(?!~)(?=punctSpace|$)|(?!~)punctSpace(~~?)(?=notPunctSpace)|[\\s](~~?)(?!~)(?=punct)|(?!~)punct(~~?)(?!~)(?=punct)|notPunctSpace(~~?)(?=notPunctSpace)",Wf=A(Yf,"gu").replace(/notPunctSpace/g,Bo).replace(/punctSpace/g,Ar).replace(/punct/g,It).getRegex(),Kf=A(/\\(punct)/,"gu").replace(/punct/g,It).getRegex(),Zf=A(/^<(scheme:[^\s\x00-\x1f<>]*|email)>/).replace("scheme",/[a-zA-Z][a-zA-Z0-9+.-]{1,31}/).replace("email",/[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+(@)[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)+(?![-_])/).getRegex(),Qf=A(za).replace("(?:-->|$)","-->").getRegex(),Jf=A("^comment|^</[a-zA-Z][\\w:-]*\\s*>|^<[a-zA-Z][\\w-]*(?:attribute)*?\\s*/?>|^<\\?[\\s\\S]*?\\?>|^<![a-zA-Z]+\\s[\\s\\S]*?>|^<!\\[CDATA\\[[\\s\\S]*?\\]\\]>").replace("comment",Qf).replace("attribute",/\s+[a-zA-Z:_][\w.:-]*(?:\s*=\s*"[^"]*"|\s*=\s*'[^']*'|\s*=\s*[^\s"'=<>`]+)?/).getRegex(),In=/(?:\[(?:\\[\s\S]|[^\[\]\\])*\]|\\[\s\S]|`+(?!`)[^`]*?`+(?!`)|``+(?=\])|[^\[\]\\`])*?/,eg=A(/^!?\[(label)\]\(\s*(href)(?:(?:[ \t]+(?:\n[ \t]*)?|\n[ \t]*)(title))?\s*\)/).replace("label",In).replace("href",/<(?:\\.|[^\n<>\\])+>|[^ \t\n\x00-\x1f]+|(?=\))/).replace("title",/"(?:\\"?|[^"\\])*"|'(?:\\'?|[^'\\])*'|\((?:\\\)?|[^)\\])*\)/).getRegex(),Dd=A(/^!?\[(label)\]\[(ref)\]/).replace("label",In).replace("ref",Ma).getRegex(),Id=A(/^!?\[(ref)\](?:\[\])?/).replace("ref",Ma).getRegex(),tg=A("reflink|nolink(?!\\()","g").replace("reflink",Dd).replace("nolink",Id).getRegex(),Gl=/[hH][tT][tT][pP][sS]?|[fF][tT][pP]/,Ra={_backpedal:ui,anyPunctuation:Kf,autolink:Zf,blockSkip:zf,br:Pd,code:Tf,del:ui,delLDelim:ui,delRDelim:ui,emStrongLDelim:Nf,emStrongRDelimAst:Ff,emStrongRDelimUnd:Vf,escape:Pf,link:eg,nolink:Id,punctuation:Af,reflink:Dd,reflinkSearch:tg,tag:Jf,text:Of,url:ui},ig={...Ra,emStrongLDelim:Bf,emStrongRDelimAst:jf,emStrongRDelimUnd:Gf,link:A(/^!?\[(label)\]\((.*?)\)/).replace("label",In).getRegex(),reflink:A(/^!?\[(label)\]\s*\[([^\]]*)\]/).replace("label",In).getRegex()},Ys={...Ra,emStrongRDelimAst:Hf,emStrongLDelim:Rf,delLDelim:Xf,delRDelim:Wf,url:A(/^((?:protocol):\/\/|www\.)(?:[a-zA-Z0-9\-]+\.?)+[^\s<]*|^email/).replace("protocol",Gl).replace("email",/[A-Za-z0-9._+-]+(@)[a-zA-Z0-9-_]+(?:\.[a-zA-Z0-9-_]*[a-zA-Z0-9])+(?![-_])/).getRegex(),_backpedal:/(?:[^?!.,:;*_'"~()&]+|\([^)]*\)|&(?![a-zA-Z0-9]+;$)|[?!.,:;*_'"~)]+(?!$))+/,del:/^(~~?)(?=[^\s~])((?:\\[\s\S]|[^\\])*?(?:\\[\s\S]|[^\s~\\]))\1(?=[^~]|$)/,text:A(/^(`+|~+|[^`~])(?:(?=[`~])|(?= {2,}\n)|(?=[a-zA-Z0-9.!#$%&'*+\/=?_`{\|}~-]+@)|[\s\S]*?(?:(?=[\\<!\[`*~_]|\b_|protocol:\/\/|www\.|$)|[^ ](?= {2,}\n)|[^a-zA-Z0-9.!#$%&'*+\/=?_`{\|}~-](?=[a-zA-Z0-9.!#$%&'*+\/=?_`{\|}~-]+@)))/).replace("protocol",Gl).getRegex()},rg={...Ys,br:A(Pd).replace("{2,}","*").getRegex(),text:A(Ys.text).replace("\\b_","\\b_| {2,}\\n").replace(/\{2,\}/g,"*").getRegex()},en={normal:Na,gfm:Cf,pedantic:Ef},jr={normal:Ra,gfm:Ys,breaks:rg,pedantic:ig},og={"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"},Xl=e=>og[e];function rt(e,t){if(t){if(se.escapeTest.test(e))return e.replace(se.escapeReplace,Xl)}else if(se.escapeTestNoEncode.test(e))return e.replace(se.escapeReplaceNoEncode,Xl);return e}function Yl(e){try{e=encodeURI(e).replace(se.percentDecode,"%")}catch{return null}return e}function Wl(e,t){var n;let i=e.replace(se.findPipe,(s,a,l)=>{let c=!1,d=a;for(;--d>=0&&l[d]==="\\";)c=!c;return c?"|":" |"}),r=i.split(se.splitPipe),o=0;if(r[0].trim()||r.shift(),r.length>0&&!((n=r.at(-1))!=null&&n.trim())&&r.pop(),t)if(r.length>t)r.splice(t);else for(;r.length<t;)r.push("");for(;o<r.length;o++)r[o]=r[o].trim().replace(se.slashPipe,"|");return r}function Nt(e,t,i){let r=e.length;if(r===0)return"";let o=0;for(;o<r&&e.charAt(r-o-1)===t;)o++;return e.slice(0,r-o)}function Kl(e){let t=e.split(`
`),i=t.length-1;for(;i>=0&&se.blankLine.test(t[i]);)i--;return t.length-i<=2?e:t.slice(0,i+1).join(`
`)}function ng(e,t){if(e.indexOf(t[1])===-1)return-1;let i=0;for(let r=0;r<e.length;r++)if(e[r]==="\\")r++;else if(e[r]===t[0])i++;else if(e[r]===t[1]&&(i--,i<0))return r;return i>0?-2:-1}function sg(e,t=0){let i=t,r="";for(let o of e)if(o==="	"){let n=4-i%4;r+=" ".repeat(n),i+=n}else r+=o,i++;return r}function Zl(e,t,i,r,o){let n=t.href,s=t.title||null,a=e[1].replace(o.other.outputLinkReplace,"$1");r.state.inLink=!0;let l={type:e[0].charAt(0)==="!"?"image":"link",raw:i,href:n,title:s,text:a,tokens:r.inlineTokens(a)};return r.state.inLink=!1,l}function ag(e,t,i){let r=e.match(i.other.indentCodeCompensation);if(r===null)return t;let o=r[1];return t.split(`
`).map(n=>{let s=n.match(i.other.beginningSpace);if(s===null)return n;let[a]=s;return a.length>=o.length?n.slice(o.length):n}).join(`
`)}var Mn=class{constructor(e){L(this,"options");L(this,"rules");L(this,"lexer");this.options=e||Ri}space(e){let t=this.rules.block.newline.exec(e);if(t&&t[0].length>0)return{type:"space",raw:t[0]}}code(e){let t=this.rules.block.code.exec(e);if(t){let i=this.options.pedantic?t[0]:Kl(t[0]),r=i.replace(this.rules.other.codeRemoveIndent,"");return{type:"code",raw:i,codeBlockStyle:"indented",text:r}}}fences(e){let t=this.rules.block.fences.exec(e);if(t){let i=t[0],r=ag(i,t[3]||"",this.rules);return{type:"code",raw:i,lang:t[2]?t[2].trim().replace(this.rules.inline.anyPunctuation,"$1"):t[2],text:r}}}heading(e){let t=this.rules.block.heading.exec(e);if(t){let i=t[2].trim();if(this.rules.other.endingHash.test(i)){let r=Nt(i,"#");(this.options.pedantic||!r||this.rules.other.endingSpaceChar.test(r))&&(i=r.trim())}return{type:"heading",raw:Nt(t[0],`
`),depth:t[1].length,text:i,tokens:this.lexer.inline(i)}}}hr(e){let t=this.rules.block.hr.exec(e);if(t)return{type:"hr",raw:Nt(t[0],`
`)}}blockquote(e){let t=this.rules.block.blockquote.exec(e);if(t){let i=Nt(t[0],`
`).split(`
`),r="",o="",n=[];for(;i.length>0;){let s=!1,a=[],l;for(l=0;l<i.length;l++)if(this.rules.other.blockquoteStart.test(i[l]))a.push(i[l]),s=!0;else if(!s)a.push(i[l]);else break;i=i.slice(l);let c=a.join(`
`),d=c.replace(this.rules.other.blockquoteSetextReplace,`
    $1`).replace(this.rules.other.blockquoteSetextReplace2,"");r=r?`${r}
${c}`:c,o=o?`${o}
${d}`:d;let h=this.lexer.state.top;if(this.lexer.state.top=!0,this.lexer.blockTokens(d,n,!0),this.lexer.state.top=h,i.length===0)break;let f=n.at(-1);if((f==null?void 0:f.type)==="code")break;if((f==null?void 0:f.type)==="blockquote"){let v=f,g=i.join(`
`),_=v.raw+`
`+g.replace(this.rules.other.blockquoteSetextReplace2,""),k=this.blockquote(_);n[n.length-1]=k,r=`${r}
${g}`,o=o.substring(0,o.length-v.text.length)+k.text;break}else if((f==null?void 0:f.type)==="list"){let v=f,g=v.raw+`
`+i.join(`
`),_=this.list(g);n[n.length-1]=_,r=r.substring(0,r.length-f.raw.length)+_.raw,o=o.substring(0,o.length-v.raw.length)+_.raw,i=g.substring(n.at(-1).raw.length).split(`
`);continue}}return{type:"blockquote",raw:r,tokens:n,text:o}}}list(e){let t=this.rules.block.list.exec(e);if(t){let i=t[1].trim(),r=i.length>1,o={type:"list",raw:"",ordered:r,start:r?+i.slice(0,-1):"",loose:!1,items:[]};i=r?`\\d{1,9}\\${i.slice(-1)}`:`\\${i}`,this.options.pedantic&&(i=r?i:"[*+-]");let n=this.rules.other.listItemRegex(i),s=!1;for(;e;){let l=!1,c="",d="";if(!(t=n.exec(e))||this.rules.block.hr.test(e))break;c=t[0],e=e.substring(c.length);let h=sg(t[2].split(`
`,1)[0],t[1].length),f=e.split(`
`,1)[0],v=!h.trim(),g=0;if(this.options.pedantic?(g=2,d=h.trimStart()):v?g=t[1].length+1:(g=h.search(this.rules.other.nonSpaceChar),g=g>4?1:g,d=h.slice(g),g+=t[1].length),v&&this.rules.other.blankLine.test(f)&&(c+=f+`
`,e=e.substring(f.length+1),l=!0),!l){let _=this.rules.other.nextBulletRegex(g),k=this.rules.other.hrRegex(g),S=this.rules.other.fencesBeginRegex(g),M=this.rules.other.headingBeginRegex(g),N=this.rules.other.htmlBeginRegex(g),R=this.rules.other.blockquoteBeginRegex(g);for(;e;){let Q=e.split(`
`,1)[0],B;if(f=Q,this.options.pedantic?(f=f.replace(this.rules.other.listReplaceNesting,"  "),B=f):B=f.replace(this.rules.other.tabCharGlobal,"    "),S.test(f)||M.test(f)||N.test(f)||R.test(f)||_.test(f)||k.test(f))break;if(B.search(this.rules.other.nonSpaceChar)>=g||!f.trim())d+=`
`+B.slice(g);else{if(v||h.replace(this.rules.other.tabCharGlobal,"    ").search(this.rules.other.nonSpaceChar)>=4||S.test(h)||M.test(h)||k.test(h))break;d+=`
`+f}v=!f.trim(),c+=Q+`
`,e=e.substring(Q.length+1),h=B.slice(g)}}o.loose||(s?o.loose=!0:this.rules.other.doubleBlankLine.test(c)&&(s=!0)),o.items.push({type:"list_item",raw:c,task:!!this.options.gfm&&this.rules.other.listIsTask.test(d),loose:!1,text:d,tokens:[]}),o.raw+=c}let a=o.items.at(-1);if(a)a.raw=a.raw.trimEnd(),a.text=a.text.trimEnd();else return;o.raw=o.raw.trimEnd();for(let l of o.items)if(this.lexer.state.top=!1,l.tokens=this.lexer.blockTokens(l.text,[]),!o.loose){let c=l.tokens.filter(h=>h.type==="space"),d=c.length>0&&c.some(h=>this.rules.other.anyLine.test(h.raw));o.loose=d}for(let l of o.items){let c=l.tokens[0];if(l.task&&((c==null?void 0:c.type)==="text"||(c==null?void 0:c.type)==="paragraph")){l.text=l.text.replace(this.rules.other.listReplaceTask,""),c.raw=c.raw.replace(this.rules.other.listReplaceTask,""),c.text=c.text.replace(this.rules.other.listReplaceTask,"");for(let h=this.lexer.inlineQueue.length-1;h>=0;h--)if(this.rules.other.listIsTask.test(this.lexer.inlineQueue[h].src)){this.lexer.inlineQueue[h].src=this.lexer.inlineQueue[h].src.replace(this.rules.other.listReplaceTask,"");break}let d=this.rules.other.listTaskCheckbox.exec(l.raw);if(d){let h={type:"checkbox",raw:d[0]+" ",checked:d[0]!=="[ ]"};l.checked=h.checked,o.loose?l.tokens[0]&&["paragraph","text"].includes(l.tokens[0].type)&&"tokens"in l.tokens[0]&&l.tokens[0].tokens?(l.tokens[0].raw=h.raw+l.tokens[0].raw,l.tokens[0].text=h.raw+l.tokens[0].text,l.tokens[0].tokens.unshift(h)):l.tokens.unshift({type:"paragraph",raw:h.raw,text:h.raw,tokens:[h]}):l.tokens.unshift(h)}}else l.task&&(l.task=!1)}if(o.loose)for(let l of o.items){l.loose=!0;for(let c of l.tokens)c.type==="text"&&(c.type="paragraph")}return o}}html(e){let t=this.rules.block.html.exec(e);if(t){let i=Kl(t[0]);return{type:"html",block:!0,raw:i,pre:t[1]==="pre"||t[1]==="script"||t[1]==="style",text:i}}}def(e){let t=this.rules.block.def.exec(e);if(t){let i=t[1].toLowerCase().replace(this.rules.other.multipleSpaceGlobal," "),r=t[2]?t[2].replace(this.rules.other.hrefBrackets,"$1").replace(this.rules.inline.anyPunctuation,"$1"):"",o=t[3]?t[3].substring(1,t[3].length-1).replace(this.rules.inline.anyPunctuation,"$1"):t[3];return{type:"def",tag:i,raw:Nt(t[0],`
`),href:r,title:o}}}table(e){var s;let t=this.rules.block.table.exec(e);if(!t||!this.rules.other.tableDelimiter.test(t[2]))return;let i=Wl(t[1]),r=t[2].replace(this.rules.other.tableAlignChars,"").split("|"),o=(s=t[3])!=null&&s.trim()?t[3].replace(this.rules.other.tableRowBlankLine,"").split(`
`):[],n={type:"table",raw:Nt(t[0],`
`),header:[],align:[],rows:[]};if(i.length===r.length){for(let a of r)this.rules.other.tableAlignRight.test(a)?n.align.push("right"):this.rules.other.tableAlignCenter.test(a)?n.align.push("center"):this.rules.other.tableAlignLeft.test(a)?n.align.push("left"):n.align.push(null);for(let a=0;a<i.length;a++)n.header.push({text:i[a],tokens:this.lexer.inline(i[a]),header:!0,align:n.align[a]});for(let a of o)n.rows.push(Wl(a,n.header.length).map((l,c)=>({text:l,tokens:this.lexer.inline(l),header:!1,align:n.align[c]})));return n}}lheading(e){let t=this.rules.block.lheading.exec(e);if(t){let i=t[1].trim();return{type:"heading",raw:Nt(t[0],`
`),depth:t[2].charAt(0)==="="?1:2,text:i,tokens:this.lexer.inline(i)}}}paragraph(e){let t=this.rules.block.paragraph.exec(e);if(t){let i=t[1].charAt(t[1].length-1)===`
`?t[1].slice(0,-1):t[1];return{type:"paragraph",raw:t[0],text:i,tokens:this.lexer.inline(i)}}}text(e){let t=this.rules.block.text.exec(e);if(t)return{type:"text",raw:t[0],text:t[0],tokens:this.lexer.inline(t[0])}}escape(e){let t=this.rules.inline.escape.exec(e);if(t)return{type:"escape",raw:t[0],text:t[1]}}tag(e){let t=this.rules.inline.tag.exec(e);if(t)return!this.lexer.state.inLink&&this.rules.other.startATag.test(t[0])?this.lexer.state.inLink=!0:this.lexer.state.inLink&&this.rules.other.endATag.test(t[0])&&(this.lexer.state.inLink=!1),!this.lexer.state.inRawBlock&&this.rules.other.startPreScriptTag.test(t[0])?this.lexer.state.inRawBlock=!0:this.lexer.state.inRawBlock&&this.rules.other.endPreScriptTag.test(t[0])&&(this.lexer.state.inRawBlock=!1),{type:"html",raw:t[0],inLink:this.lexer.state.inLink,inRawBlock:this.lexer.state.inRawBlock,block:!1,text:t[0]}}link(e){let t=this.rules.inline.link.exec(e);if(t){let i=t[2].trim();if(!this.options.pedantic&&this.rules.other.startAngleBracket.test(i)){if(!this.rules.other.endAngleBracket.test(i))return;let n=Nt(i.slice(0,-1),"\\");if((i.length-n.length)%2===0)return}else{let n=ng(t[2],"()");if(n===-2)return;if(n>-1){let s=(t[0].indexOf("!")===0?5:4)+t[1].length+n;t[2]=t[2].substring(0,n),t[0]=t[0].substring(0,s).trim(),t[3]=""}}let r=t[2],o="";if(this.options.pedantic){let n=this.rules.other.pedanticHrefTitle.exec(r);n&&(r=n[1],o=n[3])}else o=t[3]?t[3].slice(1,-1):"";return r=r.trim(),this.rules.other.startAngleBracket.test(r)&&(this.options.pedantic&&!this.rules.other.endAngleBracket.test(i)?r=r.slice(1):r=r.slice(1,-1)),Zl(t,{href:r&&r.replace(this.rules.inline.anyPunctuation,"$1"),title:o&&o.replace(this.rules.inline.anyPunctuation,"$1")},t[0],this.lexer,this.rules)}}reflink(e,t){let i;if((i=this.rules.inline.reflink.exec(e))||(i=this.rules.inline.nolink.exec(e))){let r=(i[2]||i[1]).replace(this.rules.other.multipleSpaceGlobal," "),o=t[r.toLowerCase()];if(!o){let n=i[0].charAt(0);return{type:"text",raw:n,text:n}}return Zl(i,o,i[0],this.lexer,this.rules)}}emStrong(e,t,i=""){let r=this.rules.inline.emStrongLDelim.exec(e);if(!(!r||!r[1]&&!r[2]&&!r[3]&&!r[4]||r[4]&&i.match(this.rules.other.unicodeAlphaNumeric))&&(!(r[1]||r[3])||!i||this.rules.inline.punctuation.exec(i))){let o=[...r[0]].length-1,n,s,a=o,l=0,c=r[0][0],d=i===c,h=c==="*"?this.rules.inline.emStrongRDelimAst:this.rules.inline.emStrongRDelimUnd;for(h.lastIndex=0,t=t.slice(-1*e.length+o);(r=h.exec(t))!==null;){if(n=r[1]||r[2]||r[3]||r[4]||r[5]||r[6],!n)continue;if(s=[...n].length,r[3]||r[4]){a+=s;continue}else if(r[5]||r[6]){if(o%3&&!((o+s)%3)){l+=s;continue}if(d)break}if(a-=s,a>0)continue;s=Math.min(s,s+a+l);let f=[...r[0]][0].length,v=e.slice(0,o+r.index+f+s);if(Math.min(o,s)%2){let _=v.slice(1,-1);return{type:"em",raw:v,text:_,tokens:this.lexer.inlineTokens(_)}}let g=v.slice(2,-2);return{type:"strong",raw:v,text:g,tokens:this.lexer.inlineTokens(g)}}}}codespan(e){let t=this.rules.inline.code.exec(e);if(t){let i=t[2].replace(this.rules.other.newLineCharGlobal," "),r=this.rules.other.nonSpaceChar.test(i),o=this.rules.other.startingSpaceChar.test(i)&&this.rules.other.endingSpaceChar.test(i);return r&&o&&(i=i.substring(1,i.length-1)),{type:"codespan",raw:t[0],text:i}}}br(e){let t=this.rules.inline.br.exec(e);if(t)return{type:"br",raw:t[0]}}del(e,t,i=""){let r=this.rules.inline.delLDelim.exec(e);if(r&&(!r[1]||!i||this.rules.inline.punctuation.exec(i))){let o=[...r[0]].length-1,n,s,a=o,l=this.rules.inline.delRDelim;for(l.lastIndex=0,t=t.slice(-1*e.length+o);(r=l.exec(t))!==null;){if(n=r[1]||r[2]||r[3]||r[4]||r[5]||r[6],!n||(s=[...n].length,s!==o))continue;if(r[3]||r[4]){a+=s;continue}if(a-=s,a>0)continue;s=Math.min(s,s+a);let c=[...r[0]][0].length,d=e.slice(0,o+r.index+c+s),h=d.slice(o,-o);return{type:"del",raw:d,text:h,tokens:this.lexer.inlineTokens(h)}}}}autolink(e){let t=this.rules.inline.autolink.exec(e);if(t){let i,r;return t[2]==="@"?(i=t[1],r="mailto:"+i):(i=t[1],r=i),{type:"link",raw:t[0],text:i,href:r,tokens:[{type:"text",raw:i,text:i}]}}}url(e){var i;let t;if(t=this.rules.inline.url.exec(e)){let r,o;if(t[2]==="@")r=t[0],o="mailto:"+r;else{let n;do n=t[0],t[0]=((i=this.rules.inline._backpedal.exec(t[0]))==null?void 0:i[0])??"";while(n!==t[0]);r=t[0],t[1]==="www."?o="http://"+t[0]:o=t[0]}return{type:"link",raw:t[0],text:r,href:o,tokens:[{type:"text",raw:r,text:r}]}}}inlineText(e){let t=this.rules.inline.text.exec(e);if(t){let i=this.lexer.state.inRawBlock;return{type:"text",raw:t[0],text:t[0],escaped:i}}}},We=class Ws{constructor(t){L(this,"tokens");L(this,"options");L(this,"state");L(this,"inlineQueue");L(this,"tokenizer");this.tokens=[],this.tokens.links=Object.create(null),this.options=t||Ri,this.options.tokenizer=this.options.tokenizer||new Mn,this.tokenizer=this.options.tokenizer,this.tokenizer.options=this.options,this.tokenizer.lexer=this,this.inlineQueue=[],this.state={inLink:!1,inRawBlock:!1,top:!0};let i={other:se,block:en.normal,inline:jr.normal};this.options.pedantic?(i.block=en.pedantic,i.inline=jr.pedantic):this.options.gfm&&(i.block=en.gfm,this.options.breaks?i.inline=jr.breaks:i.inline=jr.gfm),this.tokenizer.rules=i}static get rules(){return{block:en,inline:jr}}static lex(t,i){return new Ws(i).lex(t)}static lexInline(t,i){return new Ws(i).inlineTokens(t)}lex(t){t=t.replace(se.carriageReturn,`
`),this.blockTokens(t,this.tokens);for(let i=0;i<this.inlineQueue.length;i++){let r=this.inlineQueue[i];this.inlineTokens(r.src,r.tokens)}return this.inlineQueue=[],this.tokens}blockTokens(t,i=[],r=!1){var n,s,a;this.tokenizer.lexer=this,this.options.pedantic&&(t=t.replace(se.tabCharGlobal,"    ").replace(se.spaceLine,""));let o=1/0;for(;t;){if(t.length<o)o=t.length;else{this.infiniteLoopError(t.charCodeAt(0));break}let l;if((s=(n=this.options.extensions)==null?void 0:n.block)!=null&&s.some(d=>(l=d.call({lexer:this},t,i))?(t=t.substring(l.raw.length),i.push(l),!0):!1))continue;if(l=this.tokenizer.space(t)){t=t.substring(l.raw.length);let d=i.at(-1);l.raw.length===1&&d!==void 0?d.raw+=`
`:i.push(l);continue}if(l=this.tokenizer.code(t)){t=t.substring(l.raw.length);let d=i.at(-1);(d==null?void 0:d.type)==="paragraph"||(d==null?void 0:d.type)==="text"?(d.raw+=(d.raw.endsWith(`
`)?"":`
`)+l.raw,d.text+=`
`+l.text,this.inlineQueue.at(-1).src=d.text):i.push(l);continue}if(l=this.tokenizer.fences(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.heading(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.hr(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.blockquote(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.list(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.html(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.def(t)){t=t.substring(l.raw.length);let d=i.at(-1);(d==null?void 0:d.type)==="paragraph"||(d==null?void 0:d.type)==="text"?(d.raw+=(d.raw.endsWith(`
`)?"":`
`)+l.raw,d.text+=`
`+l.raw,this.inlineQueue.at(-1).src=d.text):this.tokens.links[l.tag]||(this.tokens.links[l.tag]={href:l.href,title:l.title},i.push(l));continue}if(l=this.tokenizer.table(t)){t=t.substring(l.raw.length),i.push(l);continue}if(l=this.tokenizer.lheading(t)){t=t.substring(l.raw.length),i.push(l);continue}let c=t;if((a=this.options.extensions)!=null&&a.startBlock){let d=1/0,h=t.slice(1),f;this.options.extensions.startBlock.forEach(v=>{f=v.call({lexer:this},h),typeof f=="number"&&f>=0&&(d=Math.min(d,f))}),d<1/0&&d>=0&&(c=t.substring(0,d+1))}if(this.state.top&&(l=this.tokenizer.paragraph(c))){let d=i.at(-1);r&&(d==null?void 0:d.type)==="paragraph"?(d.raw+=(d.raw.endsWith(`
`)?"":`
`)+l.raw,d.text+=`
`+l.text,this.inlineQueue.pop(),this.inlineQueue.at(-1).src=d.text):i.push(l),r=c.length!==t.length,t=t.substring(l.raw.length);continue}if(l=this.tokenizer.text(t)){t=t.substring(l.raw.length);let d=i.at(-1);(d==null?void 0:d.type)==="text"?(d.raw+=(d.raw.endsWith(`
`)?"":`
`)+l.raw,d.text+=`
`+l.text,this.inlineQueue.pop(),this.inlineQueue.at(-1).src=d.text):i.push(l);continue}if(t){this.infiniteLoopError(t.charCodeAt(0));break}}return this.state.top=!0,i}inline(t,i=[]){return this.inlineQueue.push({src:t,tokens:i}),i}inlineTokens(t,i=[]){var a,l,c,d,h;this.tokenizer.lexer=this;let r=t;if(this.tokens.links){let f=Object.keys(this.tokens.links);f.length>0&&(r=r.replace(this.tokenizer.rules.inline.reflinkSearch,v=>f.includes(v.slice(v.lastIndexOf("[")+1,-1))?"["+"a".repeat(v.length-2)+"]":v))}r=r.replace(this.tokenizer.rules.inline.anyPunctuation,f=>"+".repeat(f.length)),r=r.replace(this.tokenizer.rules.inline.blockSkip,(f,v,g)=>{let _=g?g.length:0;return f.slice(0,_)+"["+"a".repeat(f.length-_-2)+"]"}),r=((l=(a=this.options.hooks)==null?void 0:a.emStrongMask)==null?void 0:l.call({lexer:this},r))??r;let o=!1,n="",s=1/0;for(;t;){if(t.length<s)s=t.length;else{this.infiniteLoopError(t.charCodeAt(0));break}o||(n=""),o=!1;let f;if((d=(c=this.options.extensions)==null?void 0:c.inline)!=null&&d.some(g=>(f=g.call({lexer:this},t,i))?(t=t.substring(f.raw.length),i.push(f),!0):!1))continue;if(f=this.tokenizer.escape(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.tag(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.link(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.reflink(t,this.tokens.links)){t=t.substring(f.raw.length);let g=i.at(-1);f.type==="text"&&(g==null?void 0:g.type)==="text"?(g.raw+=f.raw,g.text+=f.text):i.push(f);continue}if(f=this.tokenizer.emStrong(t,r,n)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.codespan(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.br(t)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.del(t,r,n)){t=t.substring(f.raw.length),i.push(f);continue}if(f=this.tokenizer.autolink(t)){t=t.substring(f.raw.length),i.push(f);continue}if(!this.state.inLink&&(f=this.tokenizer.url(t))){t=t.substring(f.raw.length),i.push(f);continue}let v=t;if((h=this.options.extensions)!=null&&h.startInline){let g=1/0,_=t.slice(1),k;this.options.extensions.startInline.forEach(S=>{k=S.call({lexer:this},_),typeof k=="number"&&k>=0&&(g=Math.min(g,k))}),g<1/0&&g>=0&&(v=t.substring(0,g+1))}if(f=this.tokenizer.inlineText(v)){t=t.substring(f.raw.length),f.raw.slice(-1)!=="_"&&(n=f.raw.slice(-1)),o=!0;let g=i.at(-1);(g==null?void 0:g.type)==="text"?(g.raw+=f.raw,g.text+=f.text):i.push(f);continue}if(t){this.infiniteLoopError(t.charCodeAt(0));break}}return i}infiniteLoopError(t){let i="Infinite loop on byte: "+t;if(this.options.silent)console.error(i);else throw new Error(i)}},zn=class{constructor(e){L(this,"options");L(this,"parser");this.options=e||Ri}space(e){return""}code({text:e,lang:t,escaped:i}){var n;let r=(n=(t||"").match(se.notSpaceStart))==null?void 0:n[0],o=e.replace(se.endingNewline,"")+`
`;return r?'<pre><code class="language-'+rt(r)+'">'+(i?o:rt(o,!0))+`</code></pre>
`:"<pre><code>"+(i?o:rt(o,!0))+`</code></pre>
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
`}strong({tokens:e}){return`<strong>${this.parser.parseInline(e)}</strong>`}em({tokens:e}){return`<em>${this.parser.parseInline(e)}</em>`}codespan({text:e}){return`<code>${rt(e,!0)}</code>`}br(e){return"<br>"}del({tokens:e}){return`<del>${this.parser.parseInline(e)}</del>`}link({href:e,title:t,tokens:i}){let r=this.parser.parseInline(i),o=Yl(e);if(o===null)return r;e=o;let n='<a href="'+e+'"';return t&&(n+=' title="'+rt(t)+'"'),n+=">"+r+"</a>",n}image({href:e,title:t,text:i,tokens:r}){r&&(i=this.parser.parseInline(r,this.parser.textRenderer));let o=Yl(e);if(o===null)return rt(i);e=o;let n=`<img src="${e}" alt="${rt(i)}"`;return t&&(n+=` title="${rt(t)}"`),n+=">",n}text(e){return"tokens"in e&&e.tokens?this.parser.parseInline(e.tokens):"escaped"in e&&e.escaped?e.text:rt(e.text)}},La=class{strong({text:e}){return e}em({text:e}){return e}codespan({text:e}){return e}del({text:e}){return e}html({text:e}){return e}text({text:e}){return e}link({text:e}){return""+e}image({text:e}){return""+e}br(){return""}checkbox({raw:e}){return e}},Ke=class Ks{constructor(t){L(this,"options");L(this,"renderer");L(this,"textRenderer");this.options=t||Ri,this.options.renderer=this.options.renderer||new zn,this.renderer=this.options.renderer,this.renderer.options=this.options,this.renderer.parser=this,this.textRenderer=new La}static parse(t,i){return new Ks(i).parse(t)}static parseInline(t,i){return new Ks(i).parseInline(t)}parse(t){var r,o;this.renderer.parser=this;let i="";for(let n=0;n<t.length;n++){let s=t[n];if((o=(r=this.options.extensions)==null?void 0:r.renderers)!=null&&o[s.type]){let l=s,c=this.options.extensions.renderers[l.type].call({parser:this},l);if(c!==!1||!["space","hr","heading","code","table","blockquote","list","checkbox","html","def","paragraph","text"].includes(l.type)){i+=c||"";continue}}let a=s;switch(a.type){case"space":{i+=this.renderer.space(a);break}case"hr":{i+=this.renderer.hr(a);break}case"heading":{i+=this.renderer.heading(a);break}case"code":{i+=this.renderer.code(a);break}case"table":{i+=this.renderer.table(a);break}case"blockquote":{i+=this.renderer.blockquote(a);break}case"list":{i+=this.renderer.list(a);break}case"checkbox":{i+=this.renderer.checkbox(a);break}case"html":{i+=this.renderer.html(a);break}case"def":{i+=this.renderer.def(a);break}case"paragraph":{i+=this.renderer.paragraph(a);break}case"text":{i+=this.renderer.text(a);break}default:{let l='Token with "'+a.type+'" type was not found.';if(this.options.silent)return console.error(l),"";throw new Error(l)}}}return i}parseInline(t,i=this.renderer){var o,n;this.renderer.parser=this;let r="";for(let s=0;s<t.length;s++){let a=t[s];if((n=(o=this.options.extensions)==null?void 0:o.renderers)!=null&&n[a.type]){let c=this.options.extensions.renderers[a.type].call({parser:this},a);if(c!==!1||!["escape","html","link","image","checkbox","strong","em","codespan","br","del","text"].includes(a.type)){r+=c||"";continue}}let l=a;switch(l.type){case"escape":{r+=i.text(l);break}case"html":{r+=i.html(l);break}case"link":{r+=i.link(l);break}case"image":{r+=i.image(l);break}case"checkbox":{r+=i.checkbox(l);break}case"strong":{r+=i.strong(l);break}case"em":{r+=i.em(l);break}case"codespan":{r+=i.codespan(l);break}case"br":{r+=i.br(l);break}case"del":{r+=i.del(l);break}case"text":{r+=i.text(l);break}default:{let c='Token with "'+l.type+'" type was not found.';if(this.options.silent)return console.error(c),"";throw new Error(c)}}}return r}},mn,Yr=(mn=class{constructor(e){L(this,"options");L(this,"block");this.options=e||Ri}preprocess(e){return e}postprocess(e){return e}processAllTokens(e){return e}emStrongMask(e){return e}provideLexer(e=this.block){return e?We.lex:We.lexInline}provideParser(e=this.block){return e?Ke.parse:Ke.parseInline}},L(mn,"passThroughHooks",new Set(["preprocess","postprocess","processAllTokens","emStrongMask"])),L(mn,"passThroughHooksRespectAsync",new Set(["preprocess","postprocess","processAllTokens"])),mn),lg=class{constructor(...e){L(this,"defaults",Aa());L(this,"options",this.setOptions);L(this,"parse",this.parseMarkdown(!0));L(this,"parseInline",this.parseMarkdown(!1));L(this,"Parser",Ke);L(this,"Renderer",zn);L(this,"TextRenderer",La);L(this,"Lexer",We);L(this,"Tokenizer",Mn);L(this,"Hooks",Yr);this.use(...e)}walkTokens(e,t){var r,o;let i=[];for(let n of e)switch(i=i.concat(t.call(this,n)),n.type){case"table":{let s=n;for(let a of s.header)i=i.concat(this.walkTokens(a.tokens,t));for(let a of s.rows)for(let l of a)i=i.concat(this.walkTokens(l.tokens,t));break}case"list":{let s=n;i=i.concat(this.walkTokens(s.items,t));break}default:{let s=n;(o=(r=this.defaults.extensions)==null?void 0:r.childTokens)!=null&&o[s.type]?this.defaults.extensions.childTokens[s.type].forEach(a=>{let l=s[a].flat(1/0);i=i.concat(this.walkTokens(l,t))}):s.tokens&&(i=i.concat(this.walkTokens(s.tokens,t)))}}return i}use(...e){let t=this.defaults.extensions||{renderers:{},childTokens:{}};return e.forEach(i=>{let r={...i};if(r.async=this.defaults.async||r.async||!1,i.extensions&&(i.extensions.forEach(o=>{if(!o.name)throw new Error("extension name required");if("renderer"in o){let n=t.renderers[o.name];n?t.renderers[o.name]=function(...s){let a=o.renderer.apply(this,s);return a===!1&&(a=n.apply(this,s)),a}:t.renderers[o.name]=o.renderer}if("tokenizer"in o){if(!o.level||o.level!=="block"&&o.level!=="inline")throw new Error("extension level must be 'block' or 'inline'");let n=t[o.level];n?n.unshift(o.tokenizer):t[o.level]=[o.tokenizer],o.start&&(o.level==="block"?t.startBlock?t.startBlock.push(o.start):t.startBlock=[o.start]:o.level==="inline"&&(t.startInline?t.startInline.push(o.start):t.startInline=[o.start]))}"childTokens"in o&&o.childTokens&&(t.childTokens[o.name]=o.childTokens)}),r.extensions=t),i.renderer){let o=this.defaults.renderer||new zn(this.defaults);for(let n in i.renderer){if(!(n in o))throw new Error(`renderer '${n}' does not exist`);if(["options","parser"].includes(n))continue;let s=n,a=i.renderer[s],l=o[s];o[s]=(...c)=>{let d=a.apply(o,c);return d===!1&&(d=l.apply(o,c)),d||""}}r.renderer=o}if(i.tokenizer){let o=this.defaults.tokenizer||new Mn(this.defaults);for(let n in i.tokenizer){if(!(n in o))throw new Error(`tokenizer '${n}' does not exist`);if(["options","rules","lexer"].includes(n))continue;let s=n,a=i.tokenizer[s],l=o[s];o[s]=(...c)=>{let d=a.apply(o,c);return d===!1&&(d=l.apply(o,c)),d}}r.tokenizer=o}if(i.hooks){let o=this.defaults.hooks||new Yr;for(let n in i.hooks){if(!(n in o))throw new Error(`hook '${n}' does not exist`);if(["options","block"].includes(n))continue;let s=n,a=i.hooks[s],l=o[s];Yr.passThroughHooks.has(n)?o[s]=c=>{if(this.defaults.async&&Yr.passThroughHooksRespectAsync.has(n))return(async()=>{let h=await a.call(o,c);return l.call(o,h)})();let d=a.call(o,c);return l.call(o,d)}:o[s]=(...c)=>{if(this.defaults.async)return(async()=>{let h=await a.apply(o,c);return h===!1&&(h=await l.apply(o,c)),h})();let d=a.apply(o,c);return d===!1&&(d=l.apply(o,c)),d}}r.hooks=o}if(i.walkTokens){let o=this.defaults.walkTokens,n=i.walkTokens;r.walkTokens=function(s){let a=[];return a.push(n.call(this,s)),o&&(a=a.concat(o.call(this,s))),a}}this.defaults={...this.defaults,...r}}),this}setOptions(e){return this.defaults={...this.defaults,...e},this}lexer(e,t){return We.lex(e,t??this.defaults)}parser(e,t){return Ke.parse(e,t??this.defaults)}parseMarkdown(e){return(t,i)=>{let r={...i},o={...this.defaults,...r},n=this.onError(!!o.silent,!!o.async);if(this.defaults.async===!0&&r.async===!1)return n(new Error("marked(): The async option was set to true by an extension. Remove async: false from the parse options object to return a Promise."));if(typeof t>"u"||t===null)return n(new Error("marked(): input parameter is undefined or null"));if(typeof t!="string")return n(new Error("marked(): input parameter is of type "+Object.prototype.toString.call(t)+", string expected"));if(o.hooks&&(o.hooks.options=o,o.hooks.block=e),o.async)return(async()=>{let s=o.hooks?await o.hooks.preprocess(t):t,a=await(o.hooks?await o.hooks.provideLexer(e):e?We.lex:We.lexInline)(s,o),l=o.hooks?await o.hooks.processAllTokens(a):a;o.walkTokens&&await Promise.all(this.walkTokens(l,o.walkTokens));let c=await(o.hooks?await o.hooks.provideParser(e):e?Ke.parse:Ke.parseInline)(l,o);return o.hooks?await o.hooks.postprocess(c):c})().catch(n);try{o.hooks&&(t=o.hooks.preprocess(t));let s=(o.hooks?o.hooks.provideLexer(e):e?We.lex:We.lexInline)(t,o);o.hooks&&(s=o.hooks.processAllTokens(s)),o.walkTokens&&this.walkTokens(s,o.walkTokens);let a=(o.hooks?o.hooks.provideParser(e):e?Ke.parse:Ke.parseInline)(s,o);return o.hooks&&(a=o.hooks.postprocess(a)),a}catch(s){return n(s)}}}onError(e,t){return i=>{if(i.message+=`
Please report this to https://github.com/markedjs/marked.`,e){let r="<p>An error occurred:</p><pre>"+rt(i.message+"",!0)+"</pre>";return t?Promise.resolve(r):r}if(t)return Promise.reject(i);throw i}}},xi=new lg;function z(e,t){return xi.parse(e,t)}z.options=z.setOptions=function(e){return xi.setOptions(e),z.defaults=xi.defaults,kd(z.defaults),z};z.getDefaults=Aa;z.defaults=Ri;function cg(...e){return xi.use(...e),z.defaults=xi.defaults,kd(z.defaults),z}z.use=cg;z.walkTokens=function(e,t){return xi.walkTokens(e,t)};z.parseInline=xi.parseInline;z.Parser=Ke;z.parser=Ke.parse;z.Renderer=zn;z.TextRenderer=La;z.Lexer=We;z.lexer=We.lex;z.Tokenizer=Mn;z.Hooks=Yr;z.parse=z;z.options;z.setOptions;z.walkTokens;z.parseInline;Ke.parse;We.lex;var Zs={exports:{}},V={},Qs={exports:{}},Li={};function Md(){var e={};return e["align-content"]=!1,e["align-items"]=!1,e["align-self"]=!1,e["alignment-adjust"]=!1,e["alignment-baseline"]=!1,e.all=!1,e["anchor-point"]=!1,e.animation=!1,e["animation-delay"]=!1,e["animation-direction"]=!1,e["animation-duration"]=!1,e["animation-fill-mode"]=!1,e["animation-iteration-count"]=!1,e["animation-name"]=!1,e["animation-play-state"]=!1,e["animation-timing-function"]=!1,e.azimuth=!1,e["backface-visibility"]=!1,e.background=!0,e["background-attachment"]=!0,e["background-clip"]=!0,e["background-color"]=!0,e["background-image"]=!0,e["background-origin"]=!0,e["background-position"]=!0,e["background-repeat"]=!0,e["background-size"]=!0,e["baseline-shift"]=!1,e.binding=!1,e.bleed=!1,e["bookmark-label"]=!1,e["bookmark-level"]=!1,e["bookmark-state"]=!1,e.border=!0,e["border-bottom"]=!0,e["border-bottom-color"]=!0,e["border-bottom-left-radius"]=!0,e["border-bottom-right-radius"]=!0,e["border-bottom-style"]=!0,e["border-bottom-width"]=!0,e["border-collapse"]=!0,e["border-color"]=!0,e["border-image"]=!0,e["border-image-outset"]=!0,e["border-image-repeat"]=!0,e["border-image-slice"]=!0,e["border-image-source"]=!0,e["border-image-width"]=!0,e["border-left"]=!0,e["border-left-color"]=!0,e["border-left-style"]=!0,e["border-left-width"]=!0,e["border-radius"]=!0,e["border-right"]=!0,e["border-right-color"]=!0,e["border-right-style"]=!0,e["border-right-width"]=!0,e["border-spacing"]=!0,e["border-style"]=!0,e["border-top"]=!0,e["border-top-color"]=!0,e["border-top-left-radius"]=!0,e["border-top-right-radius"]=!0,e["border-top-style"]=!0,e["border-top-width"]=!0,e["border-width"]=!0,e.bottom=!1,e["box-decoration-break"]=!0,e["box-shadow"]=!0,e["box-sizing"]=!0,e["box-snap"]=!0,e["box-suppress"]=!0,e["break-after"]=!0,e["break-before"]=!0,e["break-inside"]=!0,e["caption-side"]=!1,e.chains=!1,e.clear=!0,e.clip=!1,e["clip-path"]=!1,e["clip-rule"]=!1,e.color=!0,e["color-interpolation-filters"]=!0,e["column-count"]=!1,e["column-fill"]=!1,e["column-gap"]=!1,e["column-rule"]=!1,e["column-rule-color"]=!1,e["column-rule-style"]=!1,e["column-rule-width"]=!1,e["column-span"]=!1,e["column-width"]=!1,e.columns=!1,e.contain=!1,e.content=!1,e["counter-increment"]=!1,e["counter-reset"]=!1,e["counter-set"]=!1,e.crop=!1,e.cue=!1,e["cue-after"]=!1,e["cue-before"]=!1,e.cursor=!1,e.direction=!1,e.display=!0,e["display-inside"]=!0,e["display-list"]=!0,e["display-outside"]=!0,e["dominant-baseline"]=!1,e.elevation=!1,e["empty-cells"]=!1,e.filter=!1,e.flex=!1,e["flex-basis"]=!1,e["flex-direction"]=!1,e["flex-flow"]=!1,e["flex-grow"]=!1,e["flex-shrink"]=!1,e["flex-wrap"]=!1,e.float=!1,e["float-offset"]=!1,e["flood-color"]=!1,e["flood-opacity"]=!1,e["flow-from"]=!1,e["flow-into"]=!1,e.font=!0,e["font-family"]=!0,e["font-feature-settings"]=!0,e["font-kerning"]=!0,e["font-language-override"]=!0,e["font-size"]=!0,e["font-size-adjust"]=!0,e["font-stretch"]=!0,e["font-style"]=!0,e["font-synthesis"]=!0,e["font-variant"]=!0,e["font-variant-alternates"]=!0,e["font-variant-caps"]=!0,e["font-variant-east-asian"]=!0,e["font-variant-ligatures"]=!0,e["font-variant-numeric"]=!0,e["font-variant-position"]=!0,e["font-weight"]=!0,e.grid=!1,e["grid-area"]=!1,e["grid-auto-columns"]=!1,e["grid-auto-flow"]=!1,e["grid-auto-rows"]=!1,e["grid-column"]=!1,e["grid-column-end"]=!1,e["grid-column-start"]=!1,e["grid-row"]=!1,e["grid-row-end"]=!1,e["grid-row-start"]=!1,e["grid-template"]=!1,e["grid-template-areas"]=!1,e["grid-template-columns"]=!1,e["grid-template-rows"]=!1,e["hanging-punctuation"]=!1,e.height=!0,e.hyphens=!1,e.icon=!1,e["image-orientation"]=!1,e["image-resolution"]=!1,e["ime-mode"]=!1,e["initial-letters"]=!1,e["inline-box-align"]=!1,e["justify-content"]=!1,e["justify-items"]=!1,e["justify-self"]=!1,e.left=!1,e["letter-spacing"]=!0,e["lighting-color"]=!0,e["line-box-contain"]=!1,e["line-break"]=!1,e["line-grid"]=!1,e["line-height"]=!1,e["line-snap"]=!1,e["line-stacking"]=!1,e["line-stacking-ruby"]=!1,e["line-stacking-shift"]=!1,e["line-stacking-strategy"]=!1,e["list-style"]=!0,e["list-style-image"]=!0,e["list-style-position"]=!0,e["list-style-type"]=!0,e.margin=!0,e["margin-bottom"]=!0,e["margin-left"]=!0,e["margin-right"]=!0,e["margin-top"]=!0,e["marker-offset"]=!1,e["marker-side"]=!1,e.marks=!1,e.mask=!1,e["mask-box"]=!1,e["mask-box-outset"]=!1,e["mask-box-repeat"]=!1,e["mask-box-slice"]=!1,e["mask-box-source"]=!1,e["mask-box-width"]=!1,e["mask-clip"]=!1,e["mask-image"]=!1,e["mask-origin"]=!1,e["mask-position"]=!1,e["mask-repeat"]=!1,e["mask-size"]=!1,e["mask-source-type"]=!1,e["mask-type"]=!1,e["max-height"]=!0,e["max-lines"]=!1,e["max-width"]=!0,e["min-height"]=!0,e["min-width"]=!0,e["move-to"]=!1,e["nav-down"]=!1,e["nav-index"]=!1,e["nav-left"]=!1,e["nav-right"]=!1,e["nav-up"]=!1,e["object-fit"]=!1,e["object-position"]=!1,e.opacity=!1,e.order=!1,e.orphans=!1,e.outline=!1,e["outline-color"]=!1,e["outline-offset"]=!1,e["outline-style"]=!1,e["outline-width"]=!1,e.overflow=!1,e["overflow-wrap"]=!1,e["overflow-x"]=!1,e["overflow-y"]=!1,e.padding=!0,e["padding-bottom"]=!0,e["padding-left"]=!0,e["padding-right"]=!0,e["padding-top"]=!0,e.page=!1,e["page-break-after"]=!1,e["page-break-before"]=!1,e["page-break-inside"]=!1,e["page-policy"]=!1,e.pause=!1,e["pause-after"]=!1,e["pause-before"]=!1,e.perspective=!1,e["perspective-origin"]=!1,e.pitch=!1,e["pitch-range"]=!1,e["play-during"]=!1,e.position=!1,e["presentation-level"]=!1,e.quotes=!1,e["region-fragment"]=!1,e.resize=!1,e.rest=!1,e["rest-after"]=!1,e["rest-before"]=!1,e.richness=!1,e.right=!1,e.rotation=!1,e["rotation-point"]=!1,e["ruby-align"]=!1,e["ruby-merge"]=!1,e["ruby-position"]=!1,e["shape-image-threshold"]=!1,e["shape-outside"]=!1,e["shape-margin"]=!1,e.size=!1,e.speak=!1,e["speak-as"]=!1,e["speak-header"]=!1,e["speak-numeral"]=!1,e["speak-punctuation"]=!1,e["speech-rate"]=!1,e.stress=!1,e["string-set"]=!1,e["tab-size"]=!1,e["table-layout"]=!1,e["text-align"]=!0,e["text-align-last"]=!0,e["text-combine-upright"]=!0,e["text-decoration"]=!0,e["text-decoration-color"]=!0,e["text-decoration-line"]=!0,e["text-decoration-skip"]=!0,e["text-decoration-style"]=!0,e["text-emphasis"]=!0,e["text-emphasis-color"]=!0,e["text-emphasis-position"]=!0,e["text-emphasis-style"]=!0,e["text-height"]=!0,e["text-indent"]=!0,e["text-justify"]=!0,e["text-orientation"]=!0,e["text-overflow"]=!0,e["text-shadow"]=!0,e["text-space-collapse"]=!0,e["text-transform"]=!0,e["text-underline-position"]=!0,e["text-wrap"]=!0,e.top=!1,e.transform=!1,e["transform-origin"]=!1,e["transform-style"]=!1,e.transition=!1,e["transition-delay"]=!1,e["transition-duration"]=!1,e["transition-property"]=!1,e["transition-timing-function"]=!1,e["unicode-bidi"]=!1,e["vertical-align"]=!1,e.visibility=!1,e["voice-balance"]=!1,e["voice-duration"]=!1,e["voice-family"]=!1,e["voice-pitch"]=!1,e["voice-range"]=!1,e["voice-rate"]=!1,e["voice-stress"]=!1,e["voice-volume"]=!1,e.volume=!1,e["white-space"]=!1,e.widows=!1,e.width=!0,e["will-change"]=!1,e["word-break"]=!0,e["word-spacing"]=!0,e["word-wrap"]=!0,e["wrap-flow"]=!1,e["wrap-through"]=!1,e["writing-mode"]=!1,e["z-index"]=!1,e}function dg(e,t,i){}function hg(e,t,i){}var ug=/javascript\s*\:/img;function pg(e,t){return ug.test(t)?"":t}Li.whiteList=Md();Li.getDefaultWhiteList=Md;Li.onAttr=dg;Li.onIgnoreAttr=hg;Li.safeAttrValue=pg;var fg={indexOf:function(e,t){var i,r;if(Array.prototype.indexOf)return e.indexOf(t);for(i=0,r=e.length;i<r;i++)if(e[i]===t)return i;return-1},forEach:function(e,t,i){var r,o;if(Array.prototype.forEach)return e.forEach(t,i);for(r=0,o=e.length;r<o;r++)t.call(i,e[r],r,e)},trim:function(e){return String.prototype.trim?e.trim():e.replace(/(^\s*)|(\s*$)/g,"")},trimRight:function(e){return String.prototype.trimRight?e.trimRight():e.replace(/(\s*$)/g,"")}},Vr=fg;function gg(e,t){e=Vr.trimRight(e),e[e.length-1]!==";"&&(e+=";");var i=e.length,r=!1,o=0,n=0,s="";function a(){if(!r){var d=Vr.trim(e.slice(o,n)),h=d.indexOf(":");if(h!==-1){var f=Vr.trim(d.slice(0,h)),v=Vr.trim(d.slice(h+1));if(f){var g=t(o,s.length,f,v,d);g&&(s+=g+"; ")}}}o=n+1}for(;n<i;n++){var l=e[n];if(l==="/"&&e[n+1]==="*"){var c=e.indexOf("*/",n+2);if(c===-1)break;n=c+1,o=n+1,r=!1}else l==="("?r=!0:l===")"?r=!1:l===";"?r||a():l===`
`&&a()}return Vr.trim(s)}var mg=gg,tn=Li,vg=mg;function Ql(e){return e==null}function bg(e){var t={};for(var i in e)t[i]=e[i];return t}function zd(e){e=bg(e||{}),e.whiteList=e.whiteList||tn.whiteList,e.onAttr=e.onAttr||tn.onAttr,e.onIgnoreAttr=e.onIgnoreAttr||tn.onIgnoreAttr,e.safeAttrValue=e.safeAttrValue||tn.safeAttrValue,this.options=e}zd.prototype.process=function(e){if(e=e||"",e=e.toString(),!e)return"";var t=this,i=t.options,r=i.whiteList,o=i.onAttr,n=i.onIgnoreAttr,s=i.safeAttrValue,a=vg(e,function(l,c,d,h,f){var v=r[d],g=!1;if(v===!0?g=v:typeof v=="function"?g=v(h):v instanceof RegExp&&(g=v.test(h)),g!==!0&&(g=!1),h=s(d,h),!!h){var _={position:c,sourcePosition:l,source:f,isWhite:g};if(g){var k=o(d,h,_);return Ql(k)?d+":"+h:k}else{var k=n(d,h,_);if(!Ql(k))return k}}});return a};var _g=zd;(function(e,t){var i=Li,r=_g;function o(s,a){var l=new r(a);return l.process(s)}t=e.exports=o,t.FilterCSS=r;for(var n in i)t[n]=i[n];typeof window<"u"&&(window.filterCSS=e.exports)})(Qs,Qs.exports);var Ba=Qs.exports,Fa={indexOf:function(e,t){var i,r;if(Array.prototype.indexOf)return e.indexOf(t);for(i=0,r=e.length;i<r;i++)if(e[i]===t)return i;return-1},forEach:function(e,t,i){var r,o;if(Array.prototype.forEach)return e.forEach(t,i);for(r=0,o=e.length;r<o;r++)t.call(i,e[r],r,e)},trim:function(e){return String.prototype.trim?e.trim():e.replace(/(^\s*)|(\s*$)/g,"")},spaceIndex:function(e){var t=/\s|\n|\t/,i=t.exec(e);return i?i.index:-1}},yg=Ba.FilterCSS,xg=Ba.getDefaultWhiteList,Nn=Fa;function Nd(){return{a:["target","href","title"],abbr:["title"],address:[],area:["shape","coords","href","alt"],article:[],aside:[],audio:["autoplay","controls","crossorigin","loop","muted","preload","src"],b:[],bdi:["dir"],bdo:["dir"],big:[],blockquote:["cite"],br:[],caption:[],center:[],cite:[],code:[],col:["align","valign","span","width"],colgroup:["align","valign","span","width"],dd:[],del:["datetime"],details:["open"],div:[],dl:[],dt:[],em:[],figcaption:[],figure:[],font:["color","size","face"],footer:[],h1:[],h2:[],h3:[],h4:[],h5:[],h6:[],header:[],hr:[],i:[],img:["src","alt","title","width","height","loading"],ins:["datetime"],kbd:[],li:[],mark:[],nav:[],ol:[],p:[],pre:[],s:[],section:[],small:[],span:[],sub:[],summary:[],sup:[],strong:[],strike:[],table:["width","border","align","valign"],tbody:["align","valign"],td:["width","rowspan","colspan","align","valign"],tfoot:["align","valign"],th:["width","rowspan","colspan","align","valign"],thead:["align","valign"],tr:["rowspan","align","valign"],tt:[],u:[],ul:[],video:["autoplay","controls","crossorigin","loop","muted","playsinline","poster","preload","src","height","width"]}}var Rd=new yg;function wg(e,t,i){}function $g(e,t,i){}function kg(e,t,i){}function Sg(e,t,i){}function Ld(e){return e.replace(Eg,"&lt;").replace(Pg,"&gt;")}function Cg(e,t,i,r){if(i=Vd(i),t==="href"||t==="src"){if(i=Nn.trim(i),i==="#")return"#";if(!(i.substr(0,7)==="http://"||i.substr(0,8)==="https://"||i.substr(0,7)==="mailto:"||i.substr(0,4)==="tel:"||i.substr(0,11)==="data:image/"||i.substr(0,6)==="ftp://"||i.substr(0,2)==="./"||i.substr(0,3)==="../"||i[0]==="#"||i[0]==="/"))return""}else if(t==="background"){if(rn.lastIndex=0,rn.test(i))return""}else if(t==="style"){if(Jl.lastIndex=0,Jl.test(i)||(ec.lastIndex=0,ec.test(i)&&(rn.lastIndex=0,rn.test(i))))return"";r!==!1&&(r=r||Rd,i=r.process(i))}return i=qd(i),i}var Eg=/</g,Pg=/>/g,Tg=/"/g,Og=/&quot;/g,Ag=/&#([a-zA-Z0-9]*);?/gim,Dg=/&colon;?/gim,Ig=/&newline;?/gim,rn=/((j\s*a\s*v\s*a|v\s*b|l\s*i\s*v\s*e)\s*s\s*c\s*r\s*i\s*p\s*t\s*|m\s*o\s*c\s*h\s*a):/gi,Jl=/e\s*x\s*p\s*r\s*e\s*s\s*s\s*i\s*o\s*n\s*\(.*/gi,ec=/u\s*r\s*l\s*\(.*/gi;function Bd(e){return e.replace(Tg,"&quot;")}function Fd(e){return e.replace(Og,'"')}function Hd(e){return e.replace(Ag,function(i,r){return r[0]==="x"||r[0]==="X"?String.fromCharCode(parseInt(r.substr(1),16)):String.fromCharCode(parseInt(r,10))})}function Ud(e){return e.replace(Dg,":").replace(Ig," ")}function jd(e){for(var t="",i=0,r=e.length;i<r;i++)t+=e.charCodeAt(i)<32?" ":e.charAt(i);return Nn.trim(t)}function Vd(e){return e=Fd(e),e=Hd(e),e=Ud(e),e=jd(e),e}function qd(e){return e=Bd(e),e=Ld(e),e}function Mg(){return""}function zg(e,t){typeof t!="function"&&(t=function(){});var i=!Array.isArray(e);function r(s){return i?!0:Nn.indexOf(e,s)!==-1}var o=[],n=!1;return{onIgnoreTag:function(s,a,l){if(r(s))if(l.isClosing){var c="[/removed]",d=l.position+c.length;return o.push([n!==!1?n:l.position,d]),n=!1,c}else return n||(n=l.position),"[removed]";else return t(s,a,l)},remove:function(s){var a="",l=0;return Nn.forEach(o,function(c){a+=s.slice(l,c[0]),l=c[1]}),a+=s.slice(l),a}}}function Ng(e){for(var t="",i=0;i<e.length;){var r=e.indexOf("<!--",i);if(r===-1){t+=e.slice(i);break}t+=e.slice(i,r);var o=e.indexOf("-->",r);if(o===-1)break;i=o+3}return t}function Rg(e){var t=e.split("");return t=t.filter(function(i){var r=i.charCodeAt(0);return r===127?!1:r<=31?r===10||r===13:!0}),t.join("")}V.whiteList=Nd();V.getDefaultWhiteList=Nd;V.onTag=wg;V.onIgnoreTag=$g;V.onTagAttr=kg;V.onIgnoreTagAttr=Sg;V.safeAttrValue=Cg;V.escapeHtml=Ld;V.escapeQuote=Bd;V.unescapeQuote=Fd;V.escapeHtmlEntities=Hd;V.escapeDangerHtml5Entities=Ud;V.clearNonPrintableCharacter=jd;V.friendlyAttrValue=Vd;V.escapeAttrValue=qd;V.onIgnoreTagStripAll=Mg;V.StripTagBody=zg;V.stripCommentTag=Ng;V.stripBlankChar=Rg;V.attributeWrapSign='"';V.cssFilter=Rd;V.getDefaultCSSWhiteList=xg;var ss={},Lt=Fa;function Lg(e){var t=Lt.spaceIndex(e),i;return t===-1?i=e.slice(1,-1):i=e.slice(1,t+1),i=Lt.trim(i).toLowerCase(),i.slice(0,1)==="/"&&(i=i.slice(1)),i.slice(-1)==="/"&&(i=i.slice(0,-1)),i}function Bg(e){return e.slice(0,2)==="</"}function Fg(e,t,i){var r="",o=0,n=!1,s=!1,a=0,l=e.length,c="",d="";e:for(a=0;a<l;a++){var h=e.charAt(a);if(n===!1){if(h==="<"){n=a;continue}}else if(s===!1){if(h==="<"){r+=i(e.slice(o,a)),n=a,o=a;continue}if(h===">"||a===l-1){r+=i(e.slice(o,n)),d=e.slice(n,a+1),c=Lg(d),r+=t(n,r.length,c,d,Bg(d)),o=a+1,n=!1;continue}if(h==='"'||h==="'")for(var f=1,v=e.charAt(a-f);v.trim()===""||v==="=";){if(v==="="){s=h;continue e}v=e.charAt(a-++f)}}else if(h===s){s=!1;continue}}return o<l&&(r+=i(e.substr(o))),r}var Hg=/[^a-zA-Z0-9\\_:.-]/gim;function Ug(e,t){var i=0,r=0,o=[],n=!1,s=e.length;function a(f,v){if(f=Lt.trim(f),f=f.replace(Hg,"").toLowerCase(),!(f.length<1)){var g=t(f,v||"");g&&o.push(g)}}for(var l=0;l<s;l++){var c=e.charAt(l),d,h;if(n===!1&&c==="="){n=e.slice(i,l),i=l+1,r=e.charAt(i)==='"'||e.charAt(i)==="'"?i:Vg(e,l+1);continue}if(n!==!1&&l===r){if(h=e.indexOf(c,l+1),h===-1)break;d=Lt.trim(e.slice(r+1,h)),a(n,d),n=!1,l=h,i=l+1;continue}if(/\s|\n|\t/.test(c))if(e=e.replace(/\s|\n|\t/g," "),n===!1)if(h=jg(e,l),h===-1){d=Lt.trim(e.slice(i,l)),a(d),n=!1,i=l+1;continue}else{l=h-1;continue}else if(h=qg(e,l-1),h===-1){d=Lt.trim(e.slice(i,l)),d=tc(d),a(n,d),n=!1,i=l+1;continue}else continue}return i<e.length&&(n===!1?a(e.slice(i)):a(n,tc(Lt.trim(e.slice(i))))),Lt.trim(o.join(" "))}function jg(e,t){for(;t<e.length;t++){var i=e[t];if(i!==" ")return i==="="?t:-1}}function Vg(e,t){for(;t<e.length;t++){var i=e[t];if(i!==" ")return i==="'"||i==='"'?t:-1}}function qg(e,t){for(;t>0;t--){var i=e[t];if(i!==" ")return i==="="?t:-1}}function Gg(e){return e[0]==='"'&&e[e.length-1]==='"'||e[0]==="'"&&e[e.length-1]==="'"}function tc(e){return Gg(e)?e.substr(1,e.length-2):e}ss.parseTag=Fg;ss.parseAttr=Ug;var Xg=Ba.FilterCSS,Me=V,Gd=ss,Yg=Gd.parseTag,Wg=Gd.parseAttr,xn=Fa;function on(e){return e==null}function Kg(e){var t=xn.spaceIndex(e);if(t===-1)return{html:"",closing:e[e.length-2]==="/"};e=xn.trim(e.slice(t+1,-1));var i=e[e.length-1]==="/";return i&&(e=xn.trim(e.slice(0,-1))),{html:e,closing:i}}function Zg(e){var t={};for(var i in e)t[i]=e[i];return t}function Qg(e){var t={};for(var i in e)Array.isArray(e[i])?t[i.toLowerCase()]=e[i].map(function(r){return r.toLowerCase()}):t[i.toLowerCase()]=e[i];return t}function Xd(e){e=Zg(e||{}),e.stripIgnoreTag&&(e.onIgnoreTag&&console.error('Notes: cannot use these two options "stripIgnoreTag" and "onIgnoreTag" at the same time'),e.onIgnoreTag=Me.onIgnoreTagStripAll),e.whiteList||e.allowList?e.whiteList=Qg(e.whiteList||e.allowList):e.whiteList=Me.whiteList,this.attributeWrapSign=e.singleQuotedAttributeValue===!0?"'":Me.attributeWrapSign,e.onTag=e.onTag||Me.onTag,e.onTagAttr=e.onTagAttr||Me.onTagAttr,e.onIgnoreTag=e.onIgnoreTag||Me.onIgnoreTag,e.onIgnoreTagAttr=e.onIgnoreTagAttr||Me.onIgnoreTagAttr,e.safeAttrValue=e.safeAttrValue||Me.safeAttrValue,e.escapeHtml=e.escapeHtml||Me.escapeHtml,this.options=e,e.css===!1?this.cssFilter=!1:(e.css=e.css||{},this.cssFilter=new Xg(e.css))}Xd.prototype.process=function(e){if(e=e||"",e=e.toString(),!e)return"";var t=this,i=t.options,r=i.whiteList,o=i.onTag,n=i.onIgnoreTag,s=i.onTagAttr,a=i.onIgnoreTagAttr,l=i.safeAttrValue,c=i.escapeHtml,d=t.attributeWrapSign,h=t.cssFilter;i.stripBlankChar&&(e=Me.stripBlankChar(e)),i.allowCommentTag||(e=Me.stripCommentTag(e));var f=!1;i.stripIgnoreTagBody&&(f=Me.StripTagBody(i.stripIgnoreTagBody,n),n=f.onIgnoreTag);var v=Yg(e,function(g,_,k,S,M){var N={sourcePosition:g,position:_,isClosing:M,isWhite:Object.prototype.hasOwnProperty.call(r,k)},R=o(k,S,N);if(!on(R))return R;if(N.isWhite){if(N.isClosing)return"</"+k+">";var Q=Kg(S),B=r[k],J=Wg(Q.html,function(ae,q){var De=xn.indexOf(B,ae)!==-1,ge=s(k,ae,q,De);return on(ge)?De?(q=l(k,ae,q,h),q?ae+"="+d+q+d:ae):(ge=a(k,ae,q,De),on(ge)?void 0:ge):ge});return S="<"+k,J&&(S+=" "+J),Q.closing&&(S+=" /"),S+=">",S}else return R=n(k,S,N),on(R)?c(S):R},c);return f&&(v=f.remove(v)),v};var Jg=Xd;(function(e,t){var i=V,r=ss,o=Jg;function n(a,l){var c=new o(l);return c.process(a)}t=e.exports=n,t.filterXSS=n,t.FilterXSS=o,function(){for(var a in i)t[a]=i[a];for(var l in r)t[l]=r[l]}(),typeof window<"u"&&(window.filterXSS=e.exports);function s(){return typeof self<"u"&&typeof DedicatedWorkerGlobalScope<"u"&&self instanceof DedicatedWorkerGlobalScope}s()&&(self.filterXSS=e.exports)})(Zs,Zs.exports);var Es=Zs.exports;let Ps;function em(e,t={},i={}){Ps||(Ps={...Es.getDefaultWhiteList(),table:[...Es.getDefaultWhiteList().table??[],"role"],input:["type","disabled","checked"],"ha-icon":["icon"],"ha-svg-icon":["path"],"ha-alert":["alert-type","title"]});const r={...Ps};i.allowDataUrl&&r.a&&(r.a=[...r.a,"download"]),z.setOptions({gfm:!0,breaks:!!t.breaks,...t}),z.use({renderer:{table(...n){return`<div>${new z.Renderer().table.apply(this,n)}</div>`}}});const o=z.parse(e,{async:!1});return Es.filterXSS(o,{whiteList:r,onTagAttr:(n,s,a)=>{if(n==="input")return s==="type"&&a==="checkbox"||s==="checked"||s==="disabled"?void 0:"";if(i.allowDataUrl&&n==="a"&&s==="href"&&a.startsWith("data:"))return`href="${a}"`}})}var tm=Object.defineProperty,im=Object.getOwnPropertyDescriptor,as=(e,t,i,r)=>{for(var o=r>1?void 0:r?im(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&tm(t,i,o),o};let bo=class extends hi{constructor(){super(...arguments),this.allowSvg=!1,this.breaks=!1}createRenderRoot(){return this}update(e){super.update(e),this.content!==void 0&&e.has("content")?this._renderMarkdown():(e.has("breaks")||e.has("allowSvg"))&&this._renderMarkdown()}_renderMarkdown(){const e=em(String(this.content??""),{breaks:this.breaks,gfm:!0},{allowSvg:this.allowSvg});this.innerHTML=e,this.querySelectorAll("a").forEach(t=>{try{t.host&&t.host!==document.location.host&&(t.target="_blank",t.rel="noreferrer noopener")}catch{}})}};as([p()],bo.prototype,"content",2);as([p({attribute:"allow-svg",type:Boolean})],bo.prototype,"allowSvg",2);as([p({type:Boolean})],bo.prototype,"breaks",2);bo=as([w("ha-markdown-element")],bo);var rm=Object.defineProperty,om=Object.getOwnPropertyDescriptor,ls=(e,t,i,r)=>{for(var o=r>1?void 0:r?om(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&rm(t,i,o),o};let sr=class extends y{constructor(){super(...arguments),this.allowSvg=!1,this.breaks=!1}render(){return this.content?u`
      <ha-markdown-element
        .content=${this.content}
        .allowSvg=${this.allowSvg}
        .breaks=${this.breaks}
      ></ha-markdown-element>
    `:m}};sr.styles=$`
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
  `;ls([p()],sr.prototype,"content",2);ls([p({attribute:"allow-svg",type:Boolean})],sr.prototype,"allowSvg",2);ls([p({type:Boolean})],sr.prototype,"breaks",2);sr=ls([w("ha-markdown")],sr);var nm=Object.defineProperty,sm=Object.getOwnPropertyDescriptor,Ha=(e,t,i,r)=>{for(var o=r>1?void 0:r?sm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&nm(t,i,o),o};let ar=class extends y{static getStubConfig(){return{type:"markdown",content:`The **Markdown** card allows you to write any text. You can style it **bold**, *italicized*, ~~strikethrough~~ etc. You can do images, links, and more.

For more information see the [Markdown Cheatsheet](https://commonmark.org/help).`}}setConfig(e){if(!e.content&&e.content!=="")throw new Error("Content required");this._config=e}getGridOptions(){return{columns:"full",rows:"auto",min_columns:12,min_rows:1}}render(){if(!this._config)return m;const e=!!this._config.text_only,t=String(this._config.title??""),i=String(this._config.content??"");return u`
      <ha-card
        class=${O({"text-only":e,"with-header":!!t&&!e})}
      >
        ${t&&!e?u`<h1 class="card-header">${t}</h1>`:m}
        <ha-markdown
          breaks
          .content=${i}
        ></ha-markdown>
      </ha-card>
    `}};ar.styles=$`
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
  `;Ha([p({attribute:!1})],ar.prototype,"flow",2);Ha([b()],ar.prototype,"_config",2);ar=Ha([w("hui-markdown-card")],ar);const we=$`
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
`,$e=$`
  :host {
    --tile-color: var(--state-inactive-color);
  }

  ha-card.active {
    --tile-color: var(--state-icon-color);
  }
`;var am=Object.defineProperty,lm=Object.getOwnPropertyDescriptor,Fo=(e,t,i,r)=>{for(var o=r>1?void 0:r?lm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&am(t,i,o),o};let wi=class extends y{constructor(){super(...arguments),this.vertical=!1,this.fixedInfoHeight=!1,this.expandFeatures=!1,this._hasFeatures=!1}_handleFeaturesSlotChange(e){this._hasFeatures=e.target.assignedElements().length>0}render(){const e={content:!0,vertical:this.vertical,"fixed-info-height":this.fixedInfoHeight,"has-features":this._hasFeatures};return u`
      <div class="container">
        <div class="row">
          <div class=${O(e)}>
            <slot name="icon"></slot>
            <slot name="info"></slot>
          </div>
        </div>
        <slot name="features" @slotchange=${this._handleFeaturesSlotChange}></slot>
      </div>
    `}};wi.styles=$`
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
  `;Fo([p({type:Boolean})],wi.prototype,"vertical",2);Fo([p({type:Boolean,attribute:"fixed-info-height"})],wi.prototype,"fixedInfoHeight",2);Fo([p({type:Boolean,attribute:"expand-features",reflect:!0})],wi.prototype,"expandFeatures",2);Fo([b()],wi.prototype,"_hasFeatures",2);wi=Fo([w("ha-tile-container")],wi);var cm=Object.defineProperty,dm=Object.getOwnPropertyDescriptor,Ho=(e,t,i,r)=>{for(var o=r>1?void 0:r?dm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&cm(t,i,o),o};let $i=class extends y{constructor(){super(...arguments),this.icon="",this.interactive=!1,this.active=!1,this.large=!1}render(){return u`
      <div
        class="container background ${this.interactive?"button":""}"
        role=${this.interactive?"button":m}
        tabindex=${this.interactive?"0":m}
      >
        <ha-icon .icon=${this.icon}></ha-icon>
        <slot></slot>
      </div>
    `}};$i.styles=$`
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
  `;Ho([p()],$i.prototype,"icon",2);Ho([p({type:Boolean,reflect:!0})],$i.prototype,"interactive",2);Ho([p({type:Boolean})],$i.prototype,"active",2);Ho([p({type:Boolean})],$i.prototype,"large",2);$i=Ho([w("ha-tile-icon")],$i);var hm=Object.defineProperty,um=Object.getOwnPropertyDescriptor,Ua=(e,t,i,r)=>{for(var o=r>1?void 0:r?um(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&hm(t,i,o),o};let _o=class extends y{constructor(){super(...arguments),this.primary="",this.secondary=""}render(){return u`
      <div class="info">
        <div class="primary">
          <span>${this.primary}</span>
        </div>
        ${this.secondary?u`
              <div class="secondary">
                <span>${this.secondary}</span>
              </div>
            `:""}
      </div>
    `}};_o.styles=$`
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
  `;Ua([p()],_o.prototype,"primary",2);Ua([p()],_o.prototype,"secondary",2);_o=Ua([w("ha-tile-info")],_o);var pm=Object.defineProperty,fm=Object.getOwnPropertyDescriptor,ja=(e,t,i,r)=>{for(var o=r>1?void 0:r?fm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&pm(t,i,o),o};let lr=class extends y{static getStubConfig(){return{type:"sensor",entity:"/demo/temperature",name:"Temperature",icon:"mdi:thermometer",unit:"°C"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}render(){var h,f,v;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,i=String(this._config.name??((h=t==null?void 0:t.attributes)==null?void 0:h.friendly_name)??e??"Sensor"),r=String(this._config.icon??((f=t==null?void 0:t.attributes)==null?void 0:f.icon)??"mdi:eye"),o=String(this._config.unit??((v=t==null?void 0:t.attributes)==null?void 0:v.unit_of_measurement)??"").trim(),n=!!this._config.hide_state,s=!!this._config.vertical,a=t==null?void 0:t.state,l=n?"":a==null?"—":o?`${a} ${o}`:String(a),c=T({"--tile-color":Ta(this._config.color)}),d=s?T({"--ha-tile-info-gap":"4px","--ha-tile-info-min-height":"auto","--ha-tile-info-primary-min-height":"auto","--ha-tile-info-primary-line-height":"1.3","--ha-tile-info-align-items":"center","--ha-tile-info-text-align":"center"}):m;return u`
      <ha-card style=${c}>
        <ha-tile-container ?vertical=${s} ?fixed-info-height=${s} style=${d}>
          <ha-tile-icon slot="icon" .icon=${r}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${i} .secondary=${l}></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `}};lr.styles=[we,$e,$`
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
    `];ja([p({attribute:!1})],lr.prototype,"flow",2);ja([b()],lr.prototype,"_config",2);lr=ja([w("hui-sensor-card")],lr);var gm=Object.defineProperty,mm=Object.getOwnPropertyDescriptor,Va=(e,t,i,r)=>{for(var o=r>1?void 0:r?mm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&gm(t,i,o),o};function vm(e){if(e==null)return"—";if(typeof e=="string")return e.trim()?e:"—";if(typeof e=="boolean")return e?"是":"否";if(typeof e=="number")return String(e);if(Array.isArray(e))return e.map(t=>String(t)).join(", ")||"—";try{return JSON.stringify(e)}catch{return String(e)}}let cr=class extends y{static getStubConfig(){return{type:"label",entity:"/demo/string",name:"当前素材",icon:"mdi:label-outline"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}render(){var c,d;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,i=String(this._config.name??((c=t==null?void 0:t.attributes)==null?void 0:c.friendly_name)??e??"标签"),r=String(this._config.icon??((d=t==null?void 0:t.attributes)==null?void 0:d.icon)??"mdi:label-outline"),o=!!this._config.hide_state,n=!!this._config.vertical,s=o?"":vm(t==null?void 0:t.state),a=T({"--tile-color":Ta(this._config.color)}),l=n?T({"--ha-tile-info-gap":"4px","--ha-tile-info-min-height":"auto","--ha-tile-info-primary-min-height":"auto","--ha-tile-info-primary-line-height":"1.3","--ha-tile-info-align-items":"center","--ha-tile-info-text-align":"center"}):m;return u`
      <ha-card style=${a}>
        <ha-tile-container ?vertical=${n} ?fixed-info-height=${n} style=${l}>
          <ha-tile-icon slot="icon" .icon=${r}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${i} .secondary=${s}></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `}};cr.styles=[we,$e,$`
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
    `];Va([p({attribute:!1})],cr.prototype,"flow",2);Va([b()],cr.prototype,"_config",2);cr=Va([w("hui-label-card")],cr);var bm=Object.defineProperty,_m=Object.getOwnPropertyDescriptor,et=(e,t,i,r)=>{for(var o=r>1?void 0:r?_m(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&bm(t,i,o),o};let Ce=class extends y{constructor(){super(...arguments),this.min=0,this.max=100,this.step=1,this.disabled=!1,this.showHandle=!0,this.roundValue=!0,this.vertical=!1,this._pressed=!1,this._onPointerDown=e=>{var t;this.disabled||(e.preventDefault(),this._pressed=!0,this._pointerId=e.pointerId,(t=this._slider)==null||t.setPointerCapture(e.pointerId),this._setFromPointer(e.clientX,e.clientY))},this._onPointerMove=e=>{!this._pressed||e.pointerId!==this._pointerId||this._setFromPointer(e.clientX,e.clientY)},this._onPointerUp=e=>{var t;if(e.pointerId===this._pointerId){this._pressed=!1,this._pointerId=void 0;try{(t=this._slider)==null||t.releasePointerCapture(e.pointerId)}catch{}}},this._onKeyDown=e=>{if(this.disabled)return;let t=this.value??this.min;switch(e.key){case"ArrowRight":case"ArrowUp":t+=this.step;break;case"ArrowLeft":case"ArrowDown":t-=this.step;break;case"Home":t=this.min;break;case"End":t=this.max;break;default:return}e.preventDefault(),t=this.steppedValue(t),this.value=t,this._emitChange(t)}}boundedValue(e){return Math.min(Math.max(e,this.min),this.max)}steppedValue(e){return this.boundedValue(Math.round(e/this.step)*this.step)}percentage(){const e=this.value??this.min;return this.max===this.min?0:(this.boundedValue(e)-this.min)/(this.max-this.min)}_emitChange(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}_setFromPointer(e,t){const i=this._slider;if(!i)return;const r=i.getBoundingClientRect();let o;this.vertical?o=Math.max(0,Math.min(1,(r.bottom-t)/r.height)):o=Math.max(0,Math.min(1,(e-r.left)/r.width));const n=this.steppedValue(this.min+o*(this.max-this.min));this.value=n,this._emitChange(n)}render(){const e=this.value??this.min,t=this.roundValue?Math.round(e):e;return u`
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
            style=${T({"--value":String(this.percentage())})}
          ></div>
        </div>
      </div>
    `}};Ce.styles=$`
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
  `;et([p({type:Number})],Ce.prototype,"value",2);et([p({type:Number})],Ce.prototype,"min",2);et([p({type:Number})],Ce.prototype,"max",2);et([p({type:Number})],Ce.prototype,"step",2);et([p({type:Boolean,reflect:!0})],Ce.prototype,"disabled",2);et([p({type:Boolean,attribute:"show-handle"})],Ce.prototype,"showHandle",2);et([p({type:Boolean,attribute:"round-value"})],Ce.prototype,"roundValue",2);et([p({type:Boolean,reflect:!0})],Ce.prototype,"vertical",2);et([b()],Ce.prototype,"_pressed",2);et([zo(".slider")],Ce.prototype,"_slider",2);Ce=et([w("ha-control-slider")],Ce);var ym=Object.defineProperty,xm=Object.getOwnPropertyDescriptor,qa=(e,t,i,r)=>{for(var o=r>1?void 0:r?xm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ym(t,i,o),o};let dr=class extends y{static getStubConfig(){return{type:"slider",entity:"/demo/spotlights",name:"Spotlights",icon:"mdi:spotlight-beam",min:0,max:100,step:1}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_value(){var i,r;const e=(i=this._config)==null?void 0:i.entity;if(!e||!this.flow)return 0;const t=(r=this.flow.states[e])==null?void 0:r.state;return typeof t=="number"?t:Number(t)||0}_formatValue(e){var o,n;const t=Number(((o=this._config)==null?void 0:o.max)??100),i=Number(((n=this._config)==null?void 0:n.min)??0),r=Math.round(e);return t===100&&i===0?`${r}%`:String(r)}async _onSliderChange(e){var i;e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,e.detail.value)}render(){var g,_;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,i=Number(this._config.min??0),r=Number(this._config.max??100),o=Number(this._config.step??1),n=this._value(),s=Re(t)||n>i,a=!!this._config.vertical,l=String(this._config.name??((g=t==null?void 0:t.attributes)==null?void 0:g.friendly_name)??e??"Slider"),c=String(this._config.icon??((_=t==null?void 0:t.attributes)==null?void 0:_.icon)??"mdi:spotlight-beam"),d=this._formatValue(n),h=nf(s,this._config.color),f=T({"--tile-color":h}),v=T({"--feature-color":h??(s?"var(--state-icon-color)":"var(--state-inactive-color)")});return u`
      <ha-card class=${O({active:s})} style=${f}>
        <ha-tile-container ?vertical=${a} ?fixed-info-height=${a}>
          <ha-tile-icon slot="icon" .icon=${c} .active=${s}></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${l} .secondary=${d}></ha-tile-info>
          <div slot="features" class="feature" style=${v}>
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
    `}};dr.styles=[we,$e,$`
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
    `];qa([p({attribute:!1})],dr.prototype,"flow",2);qa([b()],dr.prototype,"_config",2);dr=qa([w("hui-slider-card")],dr);var wm=Object.defineProperty,$m=Object.getOwnPropertyDescriptor,Dr=(e,t,i,r)=>{for(var o=r>1?void 0:r?$m(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&wm(t,i,o),o};let Wt=class extends y{constructor(){super(...arguments),this.checked=!1,this.disabled=!1,this.vertical=!1,this.iconOn="mdi:lightbulb",this.iconOff="mdi:lightbulb-outline"}_toggle(){this.disabled||(this.checked=!this.checked,this.dispatchEvent(new Event("change",{bubbles:!0,composed:!0})))}_onKeyDown(e){(e.key==="Enter"||e.key===" ")&&(e.preventDefault(),this._toggle())}render(){return u`
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
    `}};Wt.styles=$`
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
  `;Dr([p({type:Boolean})],Wt.prototype,"checked",2);Dr([p({type:Boolean,reflect:!0})],Wt.prototype,"disabled",2);Dr([p({type:Boolean,reflect:!0})],Wt.prototype,"vertical",2);Dr([p({attribute:"icon-on"})],Wt.prototype,"iconOn",2);Dr([p({attribute:"icon-off"})],Wt.prototype,"iconOff",2);Wt=Dr([w("ha-control-switch")],Wt);var km=Object.defineProperty,Sm=Object.getOwnPropertyDescriptor,Ga=(e,t,i,r)=>{for(var o=r>1?void 0:r?Sm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&km(t,i,o),o};let hr=class extends y{static getStubConfig(){return{type:"switch",entity:"/demo/floor_lamp",name:"Flood light",icon:"mdi:lightbulb"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _toggle(e){var i;e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,yd(this._entityState()))}render(){var d,h;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=Re(t),r=!!this._config.vertical,o=String(this._config.name??((d=t==null?void 0:t.attributes)==null?void 0:d.friendly_name)??e??"Switch"),n=String(this._config.icon??((h=t==null?void 0:t.attributes)==null?void 0:h.icon)??"mdi:lightbulb"),s=e&&this.flow?this.flow.formatState(e):i?"开启":"关闭",a=sf(i,this._config.color),l=T({"--tile-color":a}),c=T({"--feature-color":a??(i?"var(--state-icon-color)":"var(--state-inactive-color)")});return u`
      <ha-card class=${O({active:i})} style=${l}>
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
    `}};hr.styles=[we,$e,$`
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
    `];Ga([p({attribute:!1})],hr.prototype,"flow",2);Ga([b()],hr.prototype,"_config",2);hr=Ga([w("hui-switch-card")],hr);var Cm=Object.defineProperty,Em=Object.getOwnPropertyDescriptor,Xa=(e,t,i,r)=>{for(var o=r>1?void 0:r?Em(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Cm(t,i,o),o};let ur=class extends y{static getStubConfig(){return{type:"tile",entity:"/demo/light",name:"Demo Light",icon:"mdi:lightbulb"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _toggle(e){var i;e==null||e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,yd(this._entityState()))}render(){var d,h;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=Re(t),r=String(this._config.name??((d=t==null?void 0:t.attributes)==null?void 0:d.friendly_name)??e??"Toggle"),o=String(this._config.icon??((h=t==null?void 0:t.attributes)==null?void 0:h.icon)??"mdi:help-circle"),s=!!this._config.hide_state?"":e&&this.flow?this.flow.formatState(e):"—",a=!!this._config.vertical,l=!!(this._config.show_icon_action??!0),c=T({"--tile-color":Or(i,this._config.color)});return u`
      <ha-card
        class=${O({active:i})}
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
    `}};ur.styles=[we,$e,$`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
      }
      ha-card {
        height: 100%;
        min-height: 0;
      }
    `];Xa([p({attribute:!1})],ur.prototype,"flow",2);Xa([b()],ur.prototype,"_config",2);ur=Xa([w("hui-tile-card")],ur);function Rn(e){return(e==null?void 0:e.trim())||Intl.DateTimeFormat().resolvedOptions().timeZone}function yo(){return navigator.language||"zh-CN"}function Pm(e){return e==="12"?!0:e==="24"?!1:new Intl.DateTimeFormat(yo(),{hour:"numeric"}).resolvedOptions().hour12??!1}function Yd(e){return e==="medium"||e==="large"}function Wd(e,t="small",i,r=yo()){if(!Yd(t))return"";const o=t==="large"?{weekday:"long",day:"numeric",month:"long",timeZone:Rn(i)}:{weekday:"short",day:"numeric",month:"short",timeZone:Rn(i)};return new Intl.DateTimeFormat(r,o).format(e)}function Tm(e,t,i=!1){var h;const o=new Intl.DateTimeFormat("en-US",{hour:"numeric",minute:"numeric",second:"numeric",hour12:i,timeZone:Rn(t)}).formatToParts(e),n=f=>{var v;return Number(((v=o.find(g=>g.type===f))==null?void 0:v.value)??0)},s=n("hour"),a=n("minute"),l=n("second"),c=(h=o.find(f=>f.type==="dayPeriod"))==null?void 0:h.value,d=i?s:s%12||12;return{hour:s,minute:a,second:l,hour12:d,dayPeriod:c}}var Om=Object.defineProperty,Am=Object.getOwnPropertyDescriptor,gt=(e,t,i,r)=>{for(var o=r>1?void 0:r?Am(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Om(t,i,o),o};const Dm=1e3;let Le=class extends y{constructor(){super(...arguments),this.clockSize="small",this.showSeconds=!1,this._date=""}connectedCallback(){super.connectedCallback(),this._initFormatter(),this._startTick()}disconnectedCallback(){this._stopTick(),super.disconnectedCallback()}updated(e){(e.has("timeFormat")||e.has("timeZone")||e.has("showSeconds")||e.has("clockSize"))&&(this._initFormatter(),this._tick())}_initFormatter(){const e=Pm(this.timeFormat);this._dateTimeFormat=new Intl.DateTimeFormat(yo(),{hour:e?"numeric":"2-digit",minute:"2-digit",second:"2-digit",hourCycle:e?"h12":"h23",timeZone:Rn(this.timeZone)}),this._lastDateMinute=void 0}_startTick(){this._stopTick(),this._tick(),this._tickInterval=window.setInterval(()=>this._tick(),Dm)}_stopTick(){this._tickInterval&&(clearInterval(this._tickInterval),this._tickInterval=void 0)}_tick(){var i,r,o,n;if(!this._dateTimeFormat)return;const e=new Date,t=this._dateTimeFormat.formatToParts(e);this._timeHour=(i=t.find(s=>s.type==="hour"))==null?void 0:i.value,this._timeMinute=(r=t.find(s=>s.type==="minute"))==null?void 0:r.value,this._timeSecond=this.showSeconds?(o=t.find(s=>s.type==="second"))==null?void 0:o.value:void 0,this._timeAmPm=(n=t.find(s=>s.type==="dayPeriod"))==null?void 0:n.value,this._updateDate(e)}_updateDate(e){if(!Yd(this.clockSize)){this._date="",this._lastDateMinute=void 0;return}this._timeMinute!==void 0&&this._timeMinute===this._lastDateMinute&&this._date||(this._date=Wd(e,this.clockSize,this.timeZone,yo()),this._lastDateMinute=this._timeMinute)}render(){const e=this.clockSize?`size-${this.clockSize}`:"",t=!!this._date;return u`
      <div class="clock-container">
        <div class=${O({"time-parts":!0,[e]:!!e})}>
          <span class="time-part hour">${this._timeHour??"--"}</span>
          <span class="time-part minute">${this._timeMinute??"--"}</span>
          ${this._timeSecond!==void 0?u`<span class="time-part second">${this._timeSecond}</span>`:m}
          ${this._timeAmPm!==void 0?u`<span class="time-part am-pm">${this._timeAmPm}</span>`:m}
        </div>
      </div>
      ${t?u`
            <div class="date-container">
              <div class=${O({date:!0,[e]:!!e})}>
                ${this._date}
              </div>
            </div>
          `:m}
    `}};Le.styles=$`
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
  `;gt([p({type:String})],Le.prototype,"clockSize",2);gt([p({type:Boolean})],Le.prototype,"showSeconds",2);gt([p({type:String})],Le.prototype,"timeFormat",2);gt([p({type:String})],Le.prototype,"timeZone",2);gt([b()],Le.prototype,"_timeHour",2);gt([b()],Le.prototype,"_timeMinute",2);gt([b()],Le.prototype,"_timeSecond",2);gt([b()],Le.prototype,"_timeAmPm",2);gt([b()],Le.prototype,"_date",2);Le=gt([w("ha-clock-digital")],Le);var Im=Object.defineProperty,Mm=Object.getOwnPropertyDescriptor,tt=(e,t,i,r)=>{for(var o=r>1?void 0:r?Mm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Im(t,i,o),o};const zm=6e4,Nm=[0,1,2,3],Rm=Array.from({length:12},(e,t)=>t),Lm=Array.from({length:60},(e,t)=>t);function Bm(e){const t=["","I","II","III","IV","V","VI","VII","VIII","IX","X","XI","XII"];return e<1||e>12?"":t[e]??""}let Ee=class extends y{constructor(){super(...arguments),this.clockSize="small",this.showSeconds=!1,this.border=!0,this.ticks="hour",this.faceStyle="markers",this._hourOffsetSec=0,this._minuteOffsetSec=0,this._secondOffsetSec=0,this._date="",this._handleVisibilityChange=()=>{document.hidden||(this._computeOffsets(),this._updateDate())}}connectedCallback(){super.connectedCallback(),document.addEventListener("visibilitychange",this._handleVisibilityChange),this._computeOffsets(),this._updateDate(),this._startDateTick()}disconnectedCallback(){document.removeEventListener("visibilitychange",this._handleVisibilityChange),this._stopDateTick(),super.disconnectedCallback()}updated(e){(e.has("timeZone")||e.has("clockSize"))&&(this._computeOffsets(),this._updateDate())}_startDateTick(){this._stopDateTick(),this._dateTimer=window.setInterval(()=>this._updateDate(),zm)}_stopDateTick(){this._dateTimer&&(clearInterval(this._dateTimer),this._dateTimer=void 0)}_computeOffsets(){const e=new Date,t=Tm(e,this.timeZone,!0),i=e.getMilliseconds(),r=t.second+i/1e3,o=t.hour%12;this._secondOffsetSec=r,this._minuteOffsetSec=t.minute*60+r,this._hourOffsetSec=o*3600+t.minute*60+r}_updateDate(){this._date=Wd(new Date,this.clockSize,this.timeZone,yo())}_renderIndicator(e){if(!e||this.faceStyle==="markers")return m;const t=this.faceStyle==="roman"?Bm(e):String(e);return u`<div class="number">${t}</div>`}_renderTick(e,t){let i=0,r,o="tick";return t==="quarter"?(i=e*90,r=[12,3,6,9][e],o="tick hour"):t==="hour"?(i=e*30,r=(e+11)%12+1,o="tick hour"):t==="minute"&&(i=e*6,e%5===0?(r=(e/5+11)%12+1,o="tick hour"):o="tick minute"),u`
      <div class=${o} style=${`--tick-rotation: ${i}deg`}>
        <div
          class=${O({line:!0,numbers:this.faceStyle==="numbers",roman:this.faceStyle==="roman"})}
        ></div>
        ${this._renderIndicator(r)}
      </div>
    `}render(){const e=this.clockSize?`size-${this.clockSize}`:"",t=this.ticks??"hour",i=!!this._date,r=o=>T({animationDelay:`-${o}s`});return u`
      <div class=${O({"analog-clock":!0,[e]:!!e})}>
        <div class=${O({dial:!0,"dial-border":this.border})}>
          ${t==="quarter"?Nm.map(o=>this._renderTick(o,"quarter")):t==="minute"?Lm.map(o=>this._renderTick(o,"minute")):t==="none"?m:Rm.map(o=>this._renderTick(o,"hour"))}
          ${i?u`<div class=${O({date:!0,[e]:!!e})}>
                ${this._date}
              </div>`:m}
          <div class="center-dot"></div>
          <div class="hand hour" style=${r(this._hourOffsetSec)}></div>
          <div class="hand minute" style=${r(this._minuteOffsetSec)}></div>
          ${this.showSeconds?u`<div class="hand second" style=${r(this._secondOffsetSec)}></div>`:m}
        </div>
      </div>
    `}};Ee.styles=$`
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
  `;tt([p({type:String})],Ee.prototype,"clockSize",2);tt([p({type:Boolean})],Ee.prototype,"showSeconds",2);tt([p({type:String})],Ee.prototype,"timeZone",2);tt([p({type:Boolean})],Ee.prototype,"border",2);tt([p({type:String})],Ee.prototype,"ticks",2);tt([p({type:String})],Ee.prototype,"faceStyle",2);tt([b()],Ee.prototype,"_hourOffsetSec",2);tt([b()],Ee.prototype,"_minuteOffsetSec",2);tt([b()],Ee.prototype,"_secondOffsetSec",2);tt([b()],Ee.prototype,"_date",2);Ee=tt([w("ha-clock-analog")],Ee);var Fm=Object.defineProperty,Hm=Object.getOwnPropertyDescriptor,Ya=(e,t,i,r)=>{for(var o=r>1?void 0:r?Hm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Fm(t,i,o),o};let pr=class extends y{static getStubConfig(){return{type:"clock",clock_style:"digital",clock_size:"small",time_format:"24"}}setConfig(e){this._config={clock_style:"digital",clock_size:"small",...e}}getGridOptions(){var r,o,n,s;const e=String(((r=this._config)==null?void 0:r.clock_style)??"digital"),t=String(((o=this._config)==null?void 0:o.clock_size)??"small"),i=((n=this._config)==null?void 0:n.title)!==void 0&&((s=this._config)==null?void 0:s.title)!=="";if(e==="analog")switch(t){case"medium":return{columns:6,rows:i?4:3,min_columns:5,min_rows:i?4:3};case"large":return{columns:6,rows:i?5:4,min_columns:6,min_rows:i?5:4};default:return{columns:6,rows:i?3:2,min_columns:2,min_rows:i?3:2}}switch(t){case"medium":return{columns:6,rows:i?2:1,min_columns:4,min_rows:i?2:1,max_rows:4};case"large":return{columns:6,rows:2,min_columns:6,min_rows:2,max_rows:4};default:return{columns:6,rows:i?2:1,min_columns:3,min_rows:1,max_rows:4}}}render(){if(!this._config)return m;const e=String(this._config.clock_style??"digital"),t=String(this._config.clock_size??"small"),i=this._config.title!==void 0?String(this._config.title):void 0,r=!!this._config.no_background,o=!!this._config.show_seconds,n=this._config.time_format?String(this._config.time_format):void 0,s=this._config.time_zone?String(this._config.time_zone):void 0,a=this._config.analog_options??{},l=String(a.ticks??this._config.analog_ticks??"hour"),c=a.border!==void 0?!!a.border:this._config.analog_border!==void 0?!!this._config.analog_border:!0,d=String(a.face_style??this._config.analog_face_style??"markers");return u`
      <ha-card class=${O({"no-background":r})}>
        <div class=${O({"time-wrapper":!0,[`size-${t}`]:!0})}>
          ${i!==void 0?u`<div class="time-title">${i}</div>`:m}
          ${e==="analog"?u`
                <ha-clock-analog
                  .clockSize=${t}
                  .showSeconds=${o}
                  .timeZone=${s}
                  .border=${c}
                  .ticks=${l}
                  .faceStyle=${d}
                ></ha-clock-analog>
              `:u`
                <ha-clock-digital
                  .clockSize=${t}
                  .showSeconds=${o}
                  .timeFormat=${n}
                  .timeZone=${s}
                ></ha-clock-digital>
              `}
        </div>
      </ha-card>
    `}};pr.styles=$`
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
  `;Ya([p({attribute:!1})],pr.prototype,"flow",2);Ya([b()],pr.prototype,"_config",2);pr=Ya([w("hui-clock-card")],pr);const Um={off:"关闭",heat:"舒适",cool:"制冷",heat_cool:"自动",auto:"自动",dry:"除湿",fan_only:"送风"};function Wa(e){var i;const t=((i=e==null?void 0:e.attributes)==null?void 0:i.hvac_mode)??(e==null?void 0:e.state);return typeof t=="number"?t>0?"on":"off":String(t??"off").toLowerCase()}function jm(e){const t=Wa(e);return t==="on"?"开启":Um[t]??t}function Vm(e){return typeof(e==null?void 0:e.state)=="number"?e.state>0:Wa(e)!=="off"}function qm(e){var i,r;const t=((i=e==null?void 0:e.attributes)==null?void 0:i.current_temperature)??((r=e==null?void 0:e.attributes)==null?void 0:r.current_value);if(typeof t=="number")return t;if(t!=null)return Number(t)||void 0}function Gm(e,t=0){if(typeof(e==null?void 0:e.state)=="number")return e.state;const i=(e==null?void 0:e.attributes)??{},r=i.temperature??i.target_temp_high??i.target_temp_low??i.value;return typeof r=="number"?r:r!=null&&Number(r)||t}function Xm(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.min_temp)??((o=e==null?void 0:e.attributes)==null?void 0:o.min);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:0}function Ym(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.max_temp)??((o=e==null?void 0:e.attributes)==null?void 0:o.max);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:100}function Wm(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.target_temp_step)??((o=e==null?void 0:e.attributes)==null?void 0:o.step);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:1}function Km(e,t){var r;const i=(r=e==null?void 0:e.attributes)==null?void 0:r.unit_of_measurement;return typeof i=="string"&&i.trim()?i.trim():t!=null&&t.trim()?t.trim():""}function Zm(e,t=""){const i=jm(e),r=qm(e),o=t?` ${t}`:"",n=Wa(e);if(r!==void 0){const s=Number.isInteger(r)?0:1;return`${n!=="off"&&n!=="on"&&i!=="关闭"?`${i} · `:""}${r.toFixed(s)}${o}`.trim()}if(typeof(e==null?void 0:e.state)=="number"){const s=Number.isInteger(e.state)?0:1;return`${e.state.toFixed(s)}${o}`.trim()}return i}var Qm=Object.defineProperty,Jm=Object.getOwnPropertyDescriptor,ei=(e,t,i,r)=>{for(var o=r>1?void 0:r?Jm(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Qm(t,i,o),o};let ct=class extends y{constructor(){super(...arguments),this.value=0,this.min=0,this.max=100,this.step=1,this.disabled=!1,this.unit="",this.digits=0}_bounded(e){return Math.min(Math.max(e,this.min),this.max)}_stepped(e){return this._bounded(Math.round(e/this.step)*this.step)}_emit(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}_change(e,t){if(t.stopPropagation(),this.disabled)return;const i=this._stepped((this.value??this.min)+e);this.value=i,this._emit(i)}render(){const e=(this.value??this.min).toFixed(this.digits),t=this.unit?` ${this.unit}`:"";return u`
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
    `}};ct.styles=$`
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
  `;ei([p({type:Number})],ct.prototype,"value",2);ei([p({type:Number})],ct.prototype,"min",2);ei([p({type:Number})],ct.prototype,"max",2);ei([p({type:Number})],ct.prototype,"step",2);ei([p({type:Boolean})],ct.prototype,"disabled",2);ei([p({type:String})],ct.prototype,"unit",2);ei([p({type:Number})],ct.prototype,"digits",2);ct=ei([w("ha-control-number-buttons")],ct);var ev=Object.defineProperty,tv=Object.getOwnPropertyDescriptor,Ka=(e,t,i,r)=>{for(var o=r>1?void 0:r?tv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ev(t,i,o),o};function Ts(e,t){if(e==null||e==="")return t;const i=Number(e);return Number.isNaN(i)?t:i}let fr=class extends y{static getStubConfig(){return{type:"climate",entity:"/demo/climate",name:"Upstairs",icon:"mdi:home-thermometer",min:0,max:100,step:1,unit:"°C"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _onTargetChange(e){var i;e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,e.detail.value)}render(){var _,k;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=Vm(t),r=!!this._config.vertical,o=String(this._config.name??((_=t==null?void 0:t.attributes)==null?void 0:_.friendly_name)??e??"Climate"),n=String(this._config.icon??((k=t==null?void 0:t.attributes)==null?void 0:k.icon)??"mdi:home-thermometer"),s=Xm(t,Ts(this._config.min,0)),a=Ym(t,Ts(this._config.max,100)),l=Wm(t,Ts(this._config.step,1)),c=Km(t,String(this._config.unit??"")),d=Gm(t,s),h=l>=1?0:1,f=Ni(i,this._config.color,"orange"),v=T({"--tile-color":f}),g=T({"--feature-color":f??(i?"var(--orange-color, #ff9800)":"var(--state-inactive-color)")});return u`
      <ha-card class=${O({active:i})} style=${v}>
        <ha-tile-container ?vertical=${r} ?fixed-info-height=${r}>
          <ha-tile-icon slot="icon" .icon=${n} .active=${i}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${o}
            .secondary=${Zm(t,c)}
          ></ha-tile-info>
          <div slot="features" class="feature" style=${g}>
            <ha-control-number-buttons
              .value=${d}
              .min=${s}
              .max=${a}
              .step=${l}
              .unit=${c}
              .digits=${h}
              @value-changed=${this._onTargetChange}
              @click=${S=>S.stopPropagation()}
            ></ha-control-number-buttons>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};fr.styles=[we,$e,$`
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
    `];Ka([p({attribute:!1})],fr.prototype,"flow",2);Ka([b()],fr.prototype,"_config",2);fr=Ka([w("hui-climate-card")],fr);const xo=4;function Kd(e=xo){const t=Math.max(1,Math.min(12,Math.round(e)||xo));return Array.from({length:t},(i,r)=>r)}const wo=Kd(xo);function iv(e){const t=e==null?void 0:e.positions;if(Array.isArray(t)&&t.length>0){const i=t.map(r=>Number(r)).filter(r=>!Number.isNaN(r));if(i.length>0)return i}return Kd(Number((e==null?void 0:e.position_count)??xo))}function Za(e,t=wo){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.current_position)??((o=e==null?void 0:e.attributes)==null?void 0:o.position);return typeof i=="number"?Math.round(i):i!=null?Math.round(Number(i)||0):typeof(e==null?void 0:e.state)=="number"?Math.round(e.state):Re(e)?t[t.length-1]??0:t[0]??0}function rv(e,t=wo){const i=Za(e,t),r=t[0]??0;return i>r}function ov(e,t=wo){const i=Za(e,t),r=t[0]??0;return`${i>r?"已打开":"已关闭"} · ${i}`}var nv=Object.defineProperty,sv=Object.getOwnPropertyDescriptor,cs=(e,t,i,r)=>{for(var o=r>1?void 0:r?sv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&nv(t,i,o),o};let gr=class extends y{constructor(){super(...arguments),this.value=0,this.positions=wo,this.disabled=!1}_emit(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}_select(e,t){t.stopPropagation(),!this.disabled&&(this.value=e,this._emit(e))}render(){var t;const e=(t=this.positions)!=null&&t.length?this.positions:wo;return u`
      <div class="container" role="group" aria-label="单选位置">
        ${e.map(i=>u`
            <button
              type="button"
              class=${O({segment:!0,selected:this.value===i})}
              ?disabled=${this.disabled}
              @click=${r=>this._select(i,r)}
            >
              ${i}
            </button>
          `)}
      </div>
    `}};gr.styles=$`
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
  `;cs([p({type:Number})],gr.prototype,"value",2);cs([p({type:Array})],gr.prototype,"positions",2);cs([p({type:Boolean})],gr.prototype,"disabled",2);gr=cs([w("ha-control-cover-position")],gr);var av=Object.defineProperty,lv=Object.getOwnPropertyDescriptor,Qa=(e,t,i,r)=>{for(var o=r>1?void 0:r?lv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&av(t,i,o),o};let mr=class extends y{static getStubConfig(){return{type:"cover",entity:"/demo/kitchen_shutter",name:"Kitchen shutter",icon:"mdi:window-shutter",position_count:xo}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}_positions(){return iv(this._config)}async _onPositionChange(e){var i;e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,e.detail.value)}render(){var h,f;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=this._positions(),r=rv(t,i),o=!!this._config.vertical,n=String(this._config.name??((h=t==null?void 0:t.attributes)==null?void 0:h.friendly_name)??e??"Cover"),s=String(this._config.icon??((f=t==null?void 0:t.attributes)==null?void 0:f.icon)??"mdi:window-shutter"),a=Za(t,i),l=Ni(r,this._config.color,"deep-purple"),c=T({"--tile-color":l}),d=T({"--feature-color":l??(r?"var(--deep-purple-color, #7e57c2)":"var(--state-inactive-color)")});return u`
      <ha-card class=${O({active:r})} style=${c}>
        <ha-tile-container ?vertical=${o} ?fixed-info-height=${o}>
          <ha-tile-icon slot="icon" .icon=${s} .active=${r}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${n}
            .secondary=${ov(t,i)}
          ></ha-tile-info>
          <div slot="features" class="feature" style=${d}>
            <ha-control-cover-position
              .value=${a}
              .positions=${i}
              @value-changed=${this._onPositionChange}
              @click=${v=>v.stopPropagation()}
            ></ha-control-cover-position>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};mr.styles=[we,$e,$`
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
    `];Qa([p({attribute:!1})],mr.prototype,"flow",2);Qa([b()],mr.prototype,"_config",2);mr=Qa([w("hui-cover-card")],mr);var cv=Object.defineProperty,dv=Object.getOwnPropertyDescriptor,ds=(e,t,i,r)=>{for(var o=r>1?void 0:r?dv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&cv(t,i,o),o};const hv=700;let ki=class extends y{constructor(){super(...arguments),this._flashing=!1,this._lastActive=!1,this._entityReady=!1}static getStubConfig(){return{type:"trigger",entity:"/demo/trigger",name:"Trigger",icon:"mdi:gesture-tap-button"}}setConfig(e){this._config=e,this.requestUpdate(),this._syncEntitySubscription()}connectedCallback(){super.connectedCallback(),this._syncEntitySubscription()}disconnectedCallback(){this._clearEntitySubscription(),this._pulseTimer&&clearTimeout(this._pulseTimer),super.disconnectedCallback()}updated(e){e.has("flow")&&this._syncEntitySubscription()}_clearEntitySubscription(){var e;(e=this._entityUnsub)==null||e.call(this),this._entityUnsub=void 0,this._entityReady=!1}_syncEntitySubscription(){var t;this._clearEntitySubscription();const e=(t=this._config)==null?void 0:t.entity;!e||!this.flow||(this._entityUnsub=this.flow.subscribeEntity(e,i=>{const r=Re(i);this._entityReady?r&&!this._lastActive&&this._pulse():this._entityReady=!0,this._lastActive=r}))}_pulse(){this._flashing=!0,this._pulseTimer&&clearTimeout(this._pulseTimer),this._pulseTimer=setTimeout(()=>{this._flashing=!1,this._pulseTimer=void 0,this.requestUpdate()},hv),this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?2:1;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _trigger(e){var i;e==null||e.stopPropagation();const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||(await this.flow.callService(t,!0),this._pulse())}render(){var c,d;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=Re(t),r=this._flashing||i,o=String(this._config.name??((c=t==null?void 0:t.attributes)==null?void 0:c.friendly_name)??e??"Trigger"),n=String(this._config.icon??((d=t==null?void 0:t.attributes)==null?void 0:d.icon)??"mdi:gesture-tap-button"),s=!!this._config.vertical,a=r?"已触发":"未触发",l=T({"--tile-color":Or(r,this._config.color)});return u`
      <ha-card
        class=${O({active:r,pulse:this._flashing})}
        style=${l}
        tabindex="0"
        @click=${this._trigger}
        @keydown=${h=>{(h.key==="Enter"||h.key===" ")&&(h.preventDefault(),this._trigger())}}
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
    `}};ki.styles=[we,$e,$`
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
    `];ds([p({attribute:!1})],ki.prototype,"flow",2);ds([b()],ki.prototype,"_config",2);ds([b()],ki.prototype,"_flashing",2);ki=ds([w("hui-trigger-card")],ki);function Z(e,t,i){e.dispatchEvent(new CustomEvent(t,{detail:i,bubbles:!0,composed:!0}))}const Zd=`data:image/svg+xml,${encodeURIComponent(`
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
`)}`;function ot(e,t=50){if(typeof e=="number"&&Number.isFinite(e))return e;if(typeof e=="string"){const i=parseFloat(e);if(Number.isFinite(i))return i}return t}function Ln(e){return Math.min(100,Math.max(0,e))}function uv(e){var t,i;return{top:ot((t=e.style)==null?void 0:t.top,50),left:ot((i=e.style)==null?void 0:i.left,50)}}function wn(e,t,i){return{...e,style:{...e.style,top:`${Ln(t).toFixed(1)}%`,left:`${Ln(i).toFixed(1)}%`}}}function pv(e,t,i){if(!t||!e)return{value:"—",unit:"",numeric:!1};const r=e.states[t];if(!r)return{value:"—",unit:"",numeric:!1};const o=String(i??r.attributes.unit_of_measurement??"").trim(),n=r.state;if(typeof n=="boolean")return{value:n?"开":"关",unit:"",numeric:!1};if(n==null)return{value:"—",unit:"",numeric:!1};if(typeof n=="number")return{value:Number.isInteger(n)?String(n):n.toFixed(1).replace(/\.0$/,""),unit:o,numeric:!0};const s=Number(n);return n!==""&&Number.isFinite(s)?{value:Number.isInteger(s)?String(s):s.toFixed(1).replace(/\.0$/,""),unit:o,numeric:!0}:{value:String(n),unit:o,numeric:!1}}var fv=Object.defineProperty,gv=Object.getOwnPropertyDescriptor,ti=(e,t,i,r)=>{for(var o=r>1?void 0:r?gv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&fv(t,i,o),o};let dt=class extends y{constructor(){super(...arguments),this.value="—",this.unit="",this.label="",this.icon="",this.color="",this.showValue=!0,this.showIcon=!1}render(){const e=this.color?Xt(this.color):"var(--label-badge-red, #df4c1e)";return u`
      <div class="wrap" style=${T({"--badge-color":e})}>
        <div class="badge">
          ${this.showIcon&&this.icon?u`<ha-icon .icon=${this.icon}></ha-icon>`:m}
          ${this.showValue?u`
                <span class="value">${this.value}</span>
                ${this.unit?u`<span class="unit">${this.unit}</span>`:m}
              `:m}
        </div>
        ${this.label?u`<div class="label">${this.label}</div>`:m}
      </div>
    `}};dt.styles=$`
    :host {
      display: inline-flex;
    }
    .wrap {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
    }
    .badge {
      width: 56px;
      height: 56px;
      border-radius: 50%;
      box-sizing: border-box;
      border: 2px solid var(--badge-color, var(--label-badge-red, #df4c1e));
      background: var(--card-background-color, #fff);
      color: var(--primary-text-color);
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 2px;
      box-shadow: 0 1px 2px rgba(0, 0, 0, 0.12);
    }
    .value {
      font-size: 15px;
      font-weight: 600;
      line-height: 1.1;
      max-width: 50px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      text-align: center;
    }
    .unit {
      margin-top: 1px;
      font-size: 10px;
      font-weight: 600;
      line-height: 1;
      color: var(--badge-color, var(--label-badge-red, #df4c1e));
      background: color-mix(in srgb, var(--badge-color, #df4c1e) 16%, transparent);
      border-radius: 999px;
      padding: 1px 5px;
      max-width: 48px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    ha-icon {
      --mdc-icon-size: 20px;
      color: var(--badge-color, var(--label-badge-red, #df4c1e));
    }
    .label {
      font-size: 12px;
      font-weight: 500;
      color: var(--primary-text-color);
      max-width: 88px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      text-align: center;
      text-shadow: 0 0 4px var(--card-background-color, #fff);
    }
  `;ti([p()],dt.prototype,"value",2);ti([p()],dt.prototype,"unit",2);ti([p()],dt.prototype,"label",2);ti([p()],dt.prototype,"icon",2);ti([p()],dt.prototype,"color",2);ti([p({type:Boolean})],dt.prototype,"showValue",2);ti([p({type:Boolean})],dt.prototype,"showIcon",2);dt=ti([w("ha-state-badge")],dt);var mv=Object.defineProperty,vv=Object.getOwnPropertyDescriptor,Uo=(e,t,i,r)=>{for(var o=r>1?void 0:r?vv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&mv(t,i,o),o};let Si=class extends y{constructor(){super(...arguments),this.statesRevision=0,this._tick=0}connectedCallback(){super.connectedCallback(),this._syncSubscription()}disconnectedCallback(){var e;(e=this._unsub)==null||e.call(this),this._unsub=void 0,this._subscribedEntity=void 0,super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("config"))&&this._syncSubscription()}_entityId(){var e;return(e=this.config)!=null&&e.entity?String(this.config.entity).trim():""}_badgeColor(){var o;const e=(o=this.config)!=null&&o.color?String(this.config.color).trim():"",t=this._entityId(),i=t&&this.flow?this.flow.states[t]:void 0;return Re(i)?!e||e==="none"?"":e==="state"?"var(--state-icon-color)":e:"var(--state-inactive-color)"}_syncSubscription(){var t;const e=this._entityId();e===this._subscribedEntity&&this._unsub||((t=this._unsub)==null||t.call(this),this._unsub=void 0,this._subscribedEntity=void 0,!(!e||!this.flow)&&(this._subscribedEntity=e,this._unsub=this.flow.subscribeEntity(e,()=>{this._tick+=1})))}render(){var l;const e=this.config;if(!e)return m;this._tick,this.statesRevision;const t=this._entityId(),i=pv(this.flow,t,e.unit),r=t&&this.flow?this.flow.states[t]:void 0,o=String(e.icon??((l=r==null?void 0:r.attributes)==null?void 0:l.icon)??"mdi:eye"),n=this._badgeColor(),s=xd(e),a=Yt(e);return u`
      <ha-state-badge
        .value=${i.value}
        .unit=${i.unit}
        .icon=${o}
        .color=${n}
        .showIcon=${s}
        .showValue=${a}
      ></ha-state-badge>
    `}};Si.styles=$`
    :host {
      display: inline-flex;
    }
  `;Uo([p({attribute:!1})],Si.prototype,"flow",2);Uo([p({attribute:!1})],Si.prototype,"config",2);Uo([p({type:Number,attribute:!1})],Si.prototype,"statesRevision",2);Uo([b()],Si.prototype,"_tick",2);Si=Uo([w("hui-picture-state-element")],Si);var bv=Object.defineProperty,_v=Object.getOwnPropertyDescriptor,Bi=(e,t,i,r)=>{for(var o=r>1?void 0:r?_v(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&bv(t,i,o),o};let Pt=class extends y{constructor(){super(...arguments),this.preview=!1,this.statesRevision=0,this._flashing=!1,this._tick=0}connectedCallback(){super.connectedCallback(),this._syncSubscription()}disconnectedCallback(){var e;(e=this._unsub)==null||e.call(this),this._unsub=void 0,this._subscribedEntity=void 0,this._pulseTimer&&clearTimeout(this._pulseTimer),super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("config"))&&this._syncSubscription()}_entityId(){var e;return(e=this.config)!=null&&e.entity?String(this.config.entity).trim():""}_syncSubscription(){var t;const e=this._entityId();e===this._subscribedEntity&&this._unsub||((t=this._unsub)==null||t.call(this),this._unsub=void 0,this._subscribedEntity=void 0,!(!e||!this.flow)&&(this._subscribedEntity=e,this._unsub=this.flow.subscribeEntity(e,()=>{this._tick+=1})))}_circleColor(){var r;const e=this._entityId(),t=e&&this.flow?this.flow.states[e]:void 0;return this._flashing||Re(t)?Or(!0,(r=this.config)==null?void 0:r.color)??"var(--state-icon-color)":"#000"}_pulse(){this._flashing=!0,this._pulseTimer&&clearTimeout(this._pulseTimer),this._pulseTimer=setTimeout(()=>{this._flashing=!1,this._pulseTimer=void 0},700)}async _tap(e){var r,o;if(e.stopPropagation(),this.preview)return;const t=(r=this.config)!=null&&r.entity?String(this.config.entity).trim():"";if(!t||!this.flow)return;const i=rs((o=this.config)==null?void 0:o.value);await this.flow.callService(t,i),this._pulse()}render(){const e=this.config;if(!e)return m;this._tick,this.statesRevision;const t=e.icon?String(e.icon):"mdi:gesture-tap-button",i=e.text?String(e.text):e.name?String(e.name):"",r=Yt(e)&&!!i,o=this._circleColor(),n=!!(e.color&&String(e.color).trim()&&String(e.color).trim()!=="none");return u`
      <button
        type="button"
        class=${O({badge:!0,colored:n,flashing:this._flashing})}
        style=${T({"--color":o})}
        ?disabled=${this.preview||!e.entity}
        @click=${this._tap}
      >
        <span class="circle">
          <ha-icon .icon=${t}></ha-icon>
        </span>
        ${r?u`<span class="label">${i}</span>`:m}
      </button>
    `}};Pt.styles=$`
    :host {
      display: inline-flex;
    }
    .badge {
      display: inline-flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      border: none;
      background: transparent;
      padding: 0;
      cursor: pointer;
      font: inherit;
      color: var(--primary-text-color);
    }
    .circle {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
      background: var(--card-background-color, #fff);
      color: var(--color, var(--primary-color));
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.18);
      border: 2px solid color-mix(in srgb, var(--color, var(--primary-color)) 55%, transparent);
    }
    .badge.colored .circle {
      color: var(--color);
    }
    .badge.flashing .circle {
      color: #fff;
      background: var(--color, var(--primary-color));
    }
    .badge:disabled {
      cursor: grab;
    }
    .badge:not(:disabled):hover .circle {
      filter: brightness(0.96);
    }
    ha-icon {
      --mdc-icon-size: 22px;
    }
    .label {
      font-size: 12px;
      font-weight: 500;
      max-width: 88px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      text-shadow: 0 0 4px var(--card-background-color, #fff);
    }
  `;Bi([p({attribute:!1})],Pt.prototype,"flow",2);Bi([p({attribute:!1})],Pt.prototype,"config",2);Bi([p({type:Boolean})],Pt.prototype,"preview",2);Bi([p({type:Number,attribute:!1})],Pt.prototype,"statesRevision",2);Bi([b()],Pt.prototype,"_flashing",2);Bi([b()],Pt.prototype,"_tick",2);Pt=Bi([w("hui-picture-action-element")],Pt);var yv=Object.defineProperty,xv=Object.getOwnPropertyDescriptor,Ir=(e,t,i,r)=>{for(var o=r>1?void 0:r?xv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&yv(t,i,o),o};let Tt=class extends y{constructor(){super(...arguments),this.preview=!1,this._elements=[],this._dragIndex=null,this._moved=!1}static getStubConfig(){return{type:"picture-elements",image:Zd,elements:[{type:"state-badge",entity:"/demo/temperature",name:"温度",icon:"mdi:thermometer",color:"red",style:{top:"32%",left:"24%"}},{type:"button",entity:"/demo/trigger",icon:"mdi:gesture-tap-button",text:"触发",name:"触发",value:!0,style:{top:"58%",left:"72%"}}],grid_options:{columns:12,rows:"auto",min_columns:6,min_rows:3}}}setConfig(e){this._config=e,this._dragIndex===null&&(this._elements=[...e.elements??[]])}getGridOptions(){return{columns:12,rows:"auto",min_columns:6,min_rows:3}}_emitConfig(){this._config&&Z(this,"picture-elements-changed",{config:{...this._config,elements:this._elements.map(e=>({...e}))}})}_pointPercent(e){const t=this._root;if(!t)return;const i=t.getBoundingClientRect();if(!(i.width<=0||i.height<=0))return{left:Ln((e.clientX-i.left)/i.width*100),top:Ln((e.clientY-i.top)/i.height*100)}}_onElementPointerDown(e,t){!this.preview||t.button!==0||(t.preventDefault(),t.stopPropagation(),this._dragIndex=e,this._moved=!1,t.currentTarget.setPointerCapture(t.pointerId))}_onElementPointerMove(e){if(this._dragIndex===null)return;const t=this._pointPercent(e);t&&(this._moved=!0,this._elements=this._elements.map((i,r)=>r===this._dragIndex?wn(i,t.top,t.left):i))}_onElementPointerUp(e){this._dragIndex!==null&&(e.stopPropagation(),this._dragIndex=null,this._moved&&this._emitConfig())}_onRootPointerUp(e){if(!this.preview||e.button!==0||this._dragIndex!==null||this._moved)return;const t=e.target;if(t!=null&&t.closest(".element"))return;const i=this._pointPercent(e);i&&Z(this,"picture-element-position",i)}_renderElement(e,t){var n,s;const i=uv(e),o=String(e.type??"state-badge")==="button"?u`
            <hui-picture-action-element
              .flow=${this.flow}
              .config=${e}
              .preview=${this.preview}
              .statesRevision=${((n=this.flow)==null?void 0:n.statesRevision)??0}
            ></hui-picture-action-element>
          `:u`
            <hui-picture-state-element
              .flow=${this.flow}
              .config=${e}
              .statesRevision=${((s=this.flow)==null?void 0:s.statesRevision)??0}
            ></hui-picture-state-element>
          `;return u`
      <div
        class=${O({element:!0,preview:this.preview})}
        style=${T({top:`${i.top}%`,left:`${i.left}%`})}
        @pointerdown=${a=>this._onElementPointerDown(t,a)}
        @pointermove=${this._onElementPointerMove}
        @pointerup=${this._onElementPointerUp}
        @pointercancel=${this._onElementPointerUp}
      >
        ${o}
      </div>
    `}render(){var i;if(!this._config)return m;(i=this.flow)==null||i.statesRevision;const e=this._config.title?String(this._config.title):"",t=this._config.image?String(this._config.image):"";return u`
      <ha-card>
        ${e?u`<h1 class="card-header">${e}</h1>`:m}
        <div class="root" @pointerup=${this._onRootPointerUp}>
          ${t?u`<img alt="" src=${t} draggable="false" />`:u`<div class="placeholder">添加底图后可在图上放置徽章</div>`}
          ${this._elements.map((r,o)=>this._renderElement(r,o))}
        </div>
      </ha-card>
    `}};Tt.styles=$`
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
  `;Ir([p({attribute:!1})],Tt.prototype,"flow",2);Ir([p({type:Boolean})],Tt.prototype,"preview",2);Ir([b()],Tt.prototype,"_config",2);Ir([b()],Tt.prototype,"_elements",2);Ir([zo(".root")],Tt.prototype,"_root",2);Tt=Ir([w("hui-picture-elements-card")],Tt);var wv=Object.defineProperty,$v=Object.getOwnPropertyDescriptor,Ja=(e,t,i,r)=>{for(var o=r>1?void 0:r?$v(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&wv(t,i,o),o};function ic(e){const t=e.trim();if(!t)return"未设置链接";try{const i=new URL(t.includes("://")?t:`https://${t}`);return i.hostname+(i.pathname!=="/"?i.pathname:"")}catch{return t.length>48?`${t.slice(0,45)}…`:t}}function kv(e){const t=e.trim();return t?/^[a-z][a-z0-9+.-]*:/i.test(t)?t:`https://${t}`:""}let vr=class extends y{static getStubConfig(){return{type:"link",name:"超链接",url:"http://127.0.0.1:8992/",icon:"mdi:link-variant",new_tab:!0}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){return{columns:6,rows:1,min_columns:3,min_rows:1}}render(){if(!this._config)return m;const e=String(this._config.name??"超链接"),t=String(this._config.url??""),i=kv(t),r=String(this._config.icon??"mdi:link-variant"),o=this._config.new_tab!==!1,n=!i;return u`
      <ha-card class=${n?"disabled":""}>
        ${n?u`
              <div class="link-body static">
                <ha-tile-container>
                  <ha-tile-icon slot="icon" .icon=${r}></ha-tile-icon>
                  <ha-tile-info slot="info" .primary=${e} .secondary=${ic(t)}></ha-tile-info>
                </ha-tile-container>
              </div>
            `:u`
              <a
                class="link-body"
                href=${i}
                target=${o?"_blank":"_self"}
                rel=${o?"noopener noreferrer":m}
                @click=${s=>s.stopPropagation()}
              >
                <ha-tile-container>
                  <ha-tile-icon slot="icon" .icon=${r} .active=${!0}></ha-tile-icon>
                  <ha-tile-info slot="info" .primary=${e} .secondary=${ic(t)}></ha-tile-info>
                </ha-tile-container>
              </a>
            `}
      </ha-card>
    `}};vr.styles=[we,$e,$`
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
    `];Ja([p({attribute:!1})],vr.prototype,"flow",2);Ja([b()],vr.prototype,"_config",2);vr=Ja([w("hui-link-card")],vr);const Sv=-60,Cv=12,Ev=1;function rc(e,t=0){var r,o;if(typeof(e==null?void 0:e.state)=="number")return e.state;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.value)??((o=e==null?void 0:e.attributes)==null?void 0:o.gain);return typeof i=="number"?i:i!=null&&Number(i)||t}function Pv(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.min)??((o=e==null?void 0:e.attributes)==null?void 0:o.min_value);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:Sv}function Tv(e,t){var r,o;const i=((r=e==null?void 0:e.attributes)==null?void 0:r.max)??((o=e==null?void 0:e.attributes)==null?void 0:o.max_value);return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:Cv}function Ov(e,t){var r;const i=(r=e==null?void 0:e.attributes)==null?void 0:r.step;return typeof i=="number"?i:t!==void 0&&!Number.isNaN(t)?t:Ev}function Av(e){return e?"静音":"开启"}function Dv(e){if(e>=1)return 0;const t=String(e),i=t.indexOf(".");return i>=0?t.length-i-1:0}var Iv=Object.defineProperty,Mv=Object.getOwnPropertyDescriptor,hs=(e,t,i,r)=>{for(var o=r>1?void 0:r?Mv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Iv(t,i,o),o};function Os(e,t){if(e==null||e==="")return t;const i=Number(e);return Number.isNaN(i)?t:i}let Ci=class extends y{constructor(){super(...arguments),this._muted=!1}static getStubConfig(){return{type:"gain",entity:"/demo/gain",name:"输入增益",icon:"mdi:volume-high",min:-60,max:12,step:1}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_gainState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}_range(){var t,i,r;const e=this._gainState();return{min:Pv(e,Os((t=this._config)==null?void 0:t.min,-60)),max:Tv(e,Os((i=this._config)==null?void 0:i.max,12)),step:Ov(e,Os((r=this._config)==null?void 0:r.step,1))}}async _onGainChange(e){var i;if(e.stopPropagation(),this._muted)return;const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,e.detail.value)}async _toggleMute(e){var o;e.stopPropagation();const t=(o=this._config)==null?void 0:o.entity;if(!t||!this.flow)return;const{min:i}=this._range();if(this._muted){const n=this._savedGain??i;this._muted=!1,this._savedGain=void 0,await this.flow.callService(t,n);return}const r=rc(this._gainState(),i);this._savedGain=r,this._muted=!0,await this.flow.callService(t,i)}render(){var S,M;if(!this._config)return m;const e=this._config.entity,t=this._gainState(),i=this._muted,r=!i,o=!!this._config.vertical,n=String(this._config.name??((S=t==null?void 0:t.attributes)==null?void 0:S.friendly_name)??e??"增益"),s=String(this._config.icon??((M=t==null?void 0:t.attributes)==null?void 0:M.icon)??"mdi:volume-high"),{min:a,max:l,step:c}=this._range(),d=rc(t,a),h=i?a:d,f=Dv(c),v=i?"mdi:volume-off":"mdi:volume-high",g=Ni(r,this._config.color,"primary"),_=T({"--tile-color":g}),k=T({"--feature-color":g??(r?"var(--primary-color)":"var(--state-inactive-color)")});return u`
      <ha-card class=${O({active:r,muted:i})} style=${_}>
        <ha-tile-container ?vertical=${o} ?fixed-info-height=${o}>
          <ha-tile-icon slot="icon" .icon=${s} .active=${r}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${n}
            .secondary=${Av(i)}
          ></ha-tile-info>
          <div slot="features" class="feature-row" style=${k}>
            <ha-control-number-buttons
              class="gain-stepper"
              .value=${h}
              .min=${a}
              .max=${l}
              .step=${c}
              .disabled=${i}
              unit="dB"
              .digits=${f}
              @value-changed=${this._onGainChange}
              @click=${N=>N.stopPropagation()}
            ></ha-control-number-buttons>
            <button
              type="button"
              class="mute-btn"
              aria-pressed=${i?"true":"false"}
              aria-label=${i?"取消静音":"静音"}
              title=${i?"取消静音":"静音"}
              @click=${this._toggleMute}
            >
              <ha-icon .icon=${v}></ha-icon>
            </button>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Ci.styles=[we,$e,$`
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
    `];hs([p({attribute:!1})],Ci.prototype,"flow",2);hs([b()],Ci.prototype,"_config",2);hs([b()],Ci.prototype,"_muted",2);Ci=hs([w("hui-gain-card")],Ci);function Y(e){return Math.min(1,Math.max(0,e))}function Rt(e,t){const i=Number(e);return Number.isNaN(i)?0:Y(t?i/255:i)}function zv(e){const t=e.trim().replace(/^#/,"");if(!/^[0-9a-f]{3,8}$/i.test(t))return;const i=l=>l+l;let r=t;if((r.length===3||r.length===4)&&(r=[...r].map(i).join("")),r.length!==6&&r.length!==8)return;const o=Number.parseInt(r.slice(0,2),16)/255,n=Number.parseInt(r.slice(2,4),16)/255,s=Number.parseInt(r.slice(4,6),16)/255,a=r.length===8?Number.parseInt(r.slice(6,8),16)/255:1;return[o,n,s,a]}function el(e){if(Array.isArray(e)){const t=e.map(r=>Number(r));if(t.length<3||t.some(r=>Number.isNaN(r)))return;const i=t.some(r=>r>1);return[Rt(t[0],i),Rt(t[1],i),Rt(t[2],i),Rt(t.length>=4?t[3]:1,i)]}if(typeof e=="string"&&e.trim())return zv(e.trim());if(e&&typeof e=="object"){const t=e,i=t.r??t.red??t.x,r=t.g??t.green??t.y,o=t.b??t.blue??t.z;if(i!==void 0&&r!==void 0&&o!==void 0){const n=[i,r,o,t.a??t.alpha??t.w].map(s=>Number(s)).some(s=>!Number.isNaN(s)&&s>1);return[Rt(Number(i),n),Rt(Number(r),n),Rt(Number(o),n),Rt(Number(t.a??t.alpha??t.w??1),n)]}}}function Js(e){const[t,i,r,o]=e,n=Math.round(t*255),s=Math.round(i*255),a=Math.round(r*255);return o>=.999?`rgb(${n}, ${s}, ${a})`:`rgba(${n}, ${s}, ${a}, ${o.toFixed(3)})`}function Nv(e,t=!1){const i=o=>Math.round(Y(o)*255).toString(16).padStart(2,"0"),r=`#${i(e[0])}${i(e[1])}${i(e[2])}`;return t?`${r}${i(e[3])}`:r}function Qd(e){return e.map(t=>Number(t.toFixed(4)))}function Jd(e){return Nv(e,e[3]<.999).toUpperCase()}function Rv(e){return .299*e[0]+.587*e[1]+.114*e[2]>.58?"#1e293b":"#ffffff"}function ea(e){const[t,i,r,o]=e.map(Y),n=Math.max(t,i,r),s=Math.min(t,i,r),a=n-s;let l=0;a>1e-6&&(n===t?l=(i-r)/a%6:n===i?l=(r-t)/a+2:l=(t-i)/a+4,l*=60,l<0&&(l+=360));const c=n<=1e-6?0:a/n;return{h:l,s:c,v:n,a:o}}function ta(e){const t=(e.h%360+360)%360,i=Y(e.s),r=Y(e.v),o=Y(e.a),n=r*i,s=n*(1-Math.abs(t/60%2-1)),a=r-n;let l=0,c=0,d=0;return t<60?[l,c,d]=[n,s,0]:t<120?[l,c,d]=[s,n,0]:t<180?[l,c,d]=[0,n,s]:t<240?[l,c,d]=[0,s,n]:t<300?[l,c,d]=[s,0,n]:[l,c,d]=[n,0,s],[Y(l+a),Y(c+a),Y(d+a),o]}function tl(e){if(Array.isArray(e)){if(e.length<3)return;const t=Number(e[0]),i=Number(e[1]),r=Number(e[2]);return[t,i,r].some(n=>Number.isNaN(n))?void 0:{h:t>1?(t%360+360)%360:Y(t)*360,s:Y(i),v:Y(r),a:e.length>=4?Y(Number(e[3])):1}}if(e&&typeof e=="object"){const t=e,i=Number(t.h??t.hue??t.H),r=Number(t.s??t.saturation??t.S),o=Number(t.v??t.value??t.V);return[i,r,o].some(s=>Number.isNaN(s))?void 0:{h:i>1?(i%360+360)%360:Y(i)*360,s:Y(r),v:Y(o),a:Y(Number(t.a??t.alpha??1))}}}function eh(e){return[Number(Y(e.h/360).toFixed(6)),Number(Y(e.s).toFixed(6)),Number(Y(e.v).toFixed(6))]}function th(e){const t=Math.round((e.h%360+360)%360),i=Math.round(Y(e.s)*100),r=Math.round(Y(e.v)*100);return`${t}° ${i}% ${r}%`}const oc=[.12,.53,.9,1];var Lv=Object.defineProperty,Bv=Object.getOwnPropertyDescriptor,ii=(e,t,i,r)=>{for(var o=r>1?void 0:r?Bv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Lv(t,i,o),o};let ht=class extends y{constructor(){super(...arguments),this.hue=200,this.saturation=.8,this.value=1,this.alpha=1,this.disabled=!1,this._pressed=!1,this._onPointerDown=e=>{var t;this.disabled||(e.preventDefault(),this._pressed=!0,this._pointerId=e.pointerId,(t=this._pad)==null||t.setPointerCapture(e.pointerId),this._setFromPointer(e.clientX,e.clientY))},this._onPointerMove=e=>{!this._pressed||e.pointerId!==this._pointerId||this._setFromPointer(e.clientX,e.clientY)},this._onPointerUp=e=>{var t;if(e.pointerId===this._pointerId){this._pressed=!1,this._pointerId=void 0;try{(t=this._pad)==null||t.releasePointerCapture(e.pointerId)}catch{}}},this._onKeyDown=e=>{if(this.disabled)return;const t=2,i=.02;let r=this.hue,o=this.saturation;switch(e.key){case"ArrowRight":r+=t;break;case"ArrowLeft":r-=t;break;case"ArrowUp":o+=i;break;case"ArrowDown":o-=i;break;default:return}e.preventDefault(),this.hue=(r%360+360)%360,this.saturation=Math.max(0,Math.min(1,o)),this._emitChange()}}_hsva(){return{h:this.hue,s:this.saturation,v:this.value,a:this.alpha}}_rgba(){return ta(this._hsva())}_emitChange(){const e=this._rgba();this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{rgba:e,hsva:this._hsva()}}))}_setFromPointer(e,t){const i=this._pad;if(!i)return;const r=i.getBoundingClientRect();if(r.width<=0||r.height<=0)return;const o=Math.max(0,Math.min(1,(e-r.left)/r.width)),n=Math.max(0,Math.min(1,(t-r.top)/r.height));this.hue=Math.round(o*360*10)/10,this.saturation=Math.round((1-n)*1e3)/1e3,this._emitChange()}_handleStyle(){return{left:`${this.hue/360*100}%`,top:`${(1-this.saturation)*100}%`,background:Js(this._rgba())}}applyRgba(e){const t=ea(e);this.hue=t.h,this.saturation=t.s,this.value=t.v,this.alpha=t.a}render(){return u`
      <div
        class="pad"
        role="application"
        tabindex=${this.disabled?-1:0}
        aria-label="颜色触控板"
        aria-valuetext=${Js(this._rgba())}
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
          style=${T(this._handleStyle())}
          aria-hidden="true"
        ></div>
      </div>
    `}};ht.styles=$`
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
  `;ii([p({type:Number})],ht.prototype,"hue",2);ii([p({type:Number})],ht.prototype,"saturation",2);ii([p({type:Number})],ht.prototype,"value",2);ii([p({type:Number})],ht.prototype,"alpha",2);ii([p({type:Boolean,reflect:!0})],ht.prototype,"disabled",2);ii([b()],ht.prototype,"_pressed",2);ii([zo(".pad")],ht.prototype,"_pad",2);ht=ii([w("ha-control-color-pad")],ht);var Fv=Object.defineProperty,il=(e,t,i,r)=>{for(var o=void 0,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=s(t,i,o)||o);return o&&Fv(t,i,o),o};const El=class El extends y{setConfig(t){this._config=t,this._localHsva=void 0,this.requestUpdate()}getGridOptions(){var i;return{columns:6,rows:5,min_columns:!!((i=this._config)!=null&&i.vertical)?3:6,min_rows:5}}_entityState(){var i;const t=(i=this._config)==null?void 0:i.entity;if(!(!t||!this.flow))return this.flow.states[t]}_remoteHsva(){var i;const t=(i=this._entityState())==null?void 0:i.state;return this.colorMode==="hsv"?tl(t)??ea(oc):ea(el(t)??oc)}_hsva(){return this._localHsva??this._remoteHsva()}_rgba(){return ta(this._hsva())}async _send(t){var r;const i=(r=this._config)==null?void 0:r.entity;if(!(!i||!this.flow)){if(this.colorMode==="hsv"){await this.flow.callService(i,eh(t));return}await this.flow.callService(i,Qd(ta(t)))}}async _onPadChanged(t){t.stopPropagation(),this._localHsva=t.detail.hsva,await this._send(t.detail.hsva)}_onPadPointerUp(){window.setTimeout(()=>{this._localHsva=void 0},250)}async _onValueChange(t){t.stopPropagation();const i={...this._hsva(),v:t.detail.value};this._localHsva=i,await this._send(i)}async _onAlphaChange(t){t.stopPropagation();const i={...this._hsva(),a:t.detail.value};this._localHsva=i,await this._send(i)}render(){var _,k;if(!this._config)return m;const t=this._config.entity,i=this._entityState(),r=this._hsva(),o=this._rgba(),n=!!this._config.vertical,s=this.colorMode==="rgba",a=String(this._config.name??((_=i==null?void 0:i.attributes)==null?void 0:_.friendly_name)??t??this.stubName),l=String(this._config.icon??((k=i==null?void 0:i.attributes)==null?void 0:k.icon)??this.stubIcon),c=Js(o),d=this.colorMode==="hsv"?th(r):Jd(o),h=Rv(o),f=T({"--tile-color":c}),v=T({"--tile-icon-color":c,"--tile-icon-foreground":h,"--tile-icon-opacity":"1"}),g=n?T({"--ha-tile-info-gap":"4px","--ha-tile-info-min-height":"auto","--ha-tile-info-primary-min-height":"auto","--ha-tile-info-primary-line-height":"1.3"}):m;return u`
      <ha-card class=${O({active:!0})} style=${f}>
        <ha-tile-container ?vertical=${n} expand-features style=${g}>
          <ha-tile-icon
            slot="icon"
            style=${v}
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
              @click=${S=>S.stopPropagation()}
            ></ha-control-color-pad>
            <div class="sliders">
              ${s?u`
                    <div class="slider-row">
                      <span class="slider-label">透明度</span>
                      <ha-control-slider
                        .value=${r.a}
                        .min=${0}
                        .max=${1}
                        .step=${.01}
                        show-handle
                        @value-changed=${this._onAlphaChange}
                        @click=${S=>S.stopPropagation()}
                      ></ha-control-slider>
                    </div>
                  `:u`
                    <div class="slider-row">
                      <span class="slider-label">明度</span>
                      <ha-control-slider
                        .value=${r.v}
                        .min=${0}
                        .max=${1}
                        .step=${.01}
                        show-handle
                        @value-changed=${this._onValueChange}
                        @click=${S=>S.stopPropagation()}
                      ></ha-control-slider>
                    </div>
                  `}
            </div>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};El.styles=[we,$e,$`
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
    `];let Ei=El;il([p({attribute:!1})],Ei.prototype,"flow");il([b()],Ei.prototype,"_config");il([b()],Ei.prototype,"_localHsva");var Hv=Object.getOwnPropertyDescriptor,ih=(e,t,i,r)=>{for(var o=r>1?void 0:r?Hv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=s(o)||o);return o};let $o=class extends Ei{constructor(){super(...arguments),this.colorMode="rgba",this.stubEntity="/demo/rgba",this.stubName="RGBA",this.stubIcon="mdi:palette"}static getStubConfig(){return{type:"rgba",entity:"/demo/rgba",name:"RGBA",icon:"mdi:palette"}}};$o=ih([w("hui-rgba-card")],$o);let ia=class extends $o{static getStubConfig(){return{...$o.getStubConfig(),type:"color"}}};ia=ih([w("hui-color-card")],ia);var Uv=Object.getOwnPropertyDescriptor,jv=(e,t,i,r)=>{for(var o=r>1?void 0:r?Uv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=s(o)||o);return o};let ra=class extends Ei{constructor(){super(...arguments),this.colorMode="hsv",this.stubEntity="/demo/hsv",this.stubName="HSV",this.stubIcon="mdi:palette-swatch"}static getStubConfig(){return{type:"hsv",entity:"/demo/hsv",name:"HSV",icon:"mdi:palette-swatch"}}};ra=jv([w("hui-hsv-card")],ra);const wt={count:4,min:0,max:1,step:.01,max_count:16};function nc(e,t,i){return Math.min(i,Math.max(t,e))}function sc(e,t=wt.count){const i=Number(e);return Number.isFinite(i)?Math.max(1,Math.min(wt.max_count,Math.round(i))):t}function oa(e,t,i,r){var s;if(!Number.isFinite(t)||t<=0)return nc(e,i,r);const o=Math.round(e/t)*t,n=String(t).includes(".")?((s=String(t).split(".")[1])==null?void 0:s.length)??0:0;return nc(Number(o.toFixed(Math.min(n+2,8))),i,r)}function Vv(e,t){let i=[];return Array.isArray(e)?i=e.map(r=>String(r??"").trim()):typeof e=="string"&&e.trim()&&(i=e.split(/[,???|]/).map(r=>r.trim())),Array.from({length:t},(r,o)=>i[o]||String(o))}function rh(e,t,i){const r=i.fill??oa((i.min+i.max)/2,i.step,i.min,i.max),o=Array.from({length:t},()=>r);let n;if(Array.isArray(e))n=e.map(s=>Number(s));else if(typeof e=="string"&&e.trim())try{const s=JSON.parse(e);Array.isArray(s)&&(n=s.map(a=>Number(a)))}catch{const s=e.split(/[,;\s]+/).map(a=>Number(a.trim()));s.length&&!s.some(a=>Number.isNaN(a))&&(n=s)}if(!n)return o;for(let s=0;s<t;s+=1){const a=n[s];o[s]=Number.isFinite(a)?oa(a,i.step,i.min,i.max):r}return o}function oh(e){return e.map(t=>Number(t.toFixed(6)))}function qv(e,t=2){return e.map(i=>i.toFixed(t)).join(", ")}var Gv=Object.defineProperty,Xv=Object.getOwnPropertyDescriptor,us=(e,t,i,r)=>{for(var o=r>1?void 0:r?Xv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Gv(t,i,o),o};function As(e,t){if(e==null||e==="")return t;const i=Number(e);return Number.isNaN(i)?t:i}let Pi=class extends y{static getStubConfig(){return{type:"multi-fader",entity:"/demo/faders",name:"多维推杆",icon:"mdi:tune-vertical-variant",count:wt.count,min:wt.min,max:wt.max,step:wt.step,labels:"0,1,2,3",orientation:"vertical"}}setConfig(e){this._config=e,this._local=void 0,this.requestUpdate()}getGridOptions(){var o,n;const e=!!((o=this._config)!=null&&o.vertical),t=sc((n=this._config)==null?void 0:n.count);if(this._orientation()==="vertical")return{columns:Math.min(12,Math.max(6,t*2)),rows:4,min_columns:e?3:6,min_rows:3};const r=Math.max(2,Math.min(8,1+Math.ceil(t/2)));return{columns:6,rows:r,min_columns:e?3:6,min_rows:r}}_orientation(){var e;return((e=this._config)==null?void 0:e.orientation)==="horizontal"?"horizontal":"vertical"}_options(){var e,t,i,r;return{count:sc((e=this._config)==null?void 0:e.count),min:As((t=this._config)==null?void 0:t.min,wt.min),max:As((i=this._config)==null?void 0:i.max,wt.max),step:As((r=this._config)==null?void 0:r.step,wt.step)}}_values(){var s,a;const{count:e,min:t,max:i,step:r}=this._options();if(this._local&&this._local.length===e)return this._local;const o=(s=this._config)==null?void 0:s.entity,n=o&&this.flow?(a=this.flow.states[o])==null?void 0:a.state:void 0;return rh(n,e,{min:t,max:i,step:r})}async _send(e){var i;const t=(i=this._config)==null?void 0:i.entity;!t||!this.flow||await this.flow.callService(t,oh(e))}async _onFaderChange(e,t){t.stopPropagation();const{count:i,min:r,max:o,step:n}=this._options(),s=[...this._values()];for(;s.length<i;)s.push(r);s[e]=oa(t.detail.value,n,r,o),this._local=s.slice(0,i),await this._send(this._local)}_onPointerUp(){window.setTimeout(()=>{this._local=void 0},250)}render(){var N,R;if(!this._config)return m;const e=this._config.entity,t=e&&this.flow?this.flow.states[e]:void 0,{count:i,min:r,max:o,step:n}=this._options(),s=this._values(),a=Vv(this._config.labels,i),l=this._orientation(),c=!!this._config.vertical,d=String(this._config.name??((N=t==null?void 0:t.attributes)==null?void 0:N.friendly_name)??e??"多维推杆"),h=String(this._config.icon??((R=t==null?void 0:t.attributes)==null?void 0:R.icon)??"mdi:tune-vertical-variant"),f=s.some(Q=>Q>r),v=Ni(f,this._config.color,"primary"),g=T({"--tile-color":v}),_=T({"--feature-color":v??"var(--primary-color)"}),k=n<1?2:0,M=!!this._config.hide_state?"":qv(s,k);return u`
      <ha-card class=${O({active:f})} style=${g}>
        <ha-tile-container ?vertical=${c} expand-features>
          <ha-tile-icon slot="icon" .icon=${h} .active=${f}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${d}
            .secondary=${M}
          ></ha-tile-info>
          <div
            slot="features"
            class=${O({faders:!0,vertical:l==="vertical",horizontal:l==="horizontal"})}
            style=${_}
          >
            ${s.map((Q,B)=>u`
                <div class="fader">
                  ${l==="horizontal"?u`<span class="fader-label">${a[B]}</span>`:m}
                  <ha-control-slider
                    .value=${Q}
                    .min=${r}
                    .max=${o}
                    .step=${n}
                    ?vertical=${l==="vertical"}
                    show-handle
                    .roundValue=${n>=1}
                    @value-changed=${J=>this._onFaderChange(B,J)}
                    @pointerup=${this._onPointerUp}
                    @click=${J=>J.stopPropagation()}
                  ></ha-control-slider>
                  ${l==="vertical"?u`<span class="fader-label">${a[B]}</span>`:m}
                </div>
              `)}
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Pi.styles=[we,$e,$`
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
      .faders {
        display: flex;
        box-sizing: border-box;
        min-height: 0;
        height: 100%;
        --feature-height: 28px;
        --feature-border-radius: 10px;
      }
      .faders.vertical {
        flex-direction: row;
        align-items: stretch;
        justify-content: space-evenly;
        gap: 6px;
        --feature-height: 28px;
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
        gap: 4px;
      }
      .faders.vertical ha-control-slider {
        flex: 1 1 auto;
        min-height: 0;
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
    `];us([p({attribute:!1})],Pi.prototype,"flow",2);us([b()],Pi.prototype,"_config",2);us([b()],Pi.prototype,"_local",2);Pi=us([w("hui-multi-fader-card")],Pi);const le={x_min:0,x_max:1,y_min:0,y_max:1,step:.01,invert_y:!0,snap_center:!1,show_grid:!0};function ac(e,t,i){return Math.min(i,Math.max(t,e))}function lc(e,t,i,r){var a;if(!Number.isFinite(t)||t<=0)return ac(e,i,r);const o=Math.round(e/t)*t,n=String(t).includes(".")?((a=String(t).split(".")[1])==null?void 0:a.length)??0:0,s=Number(o.toFixed(Math.min(n+2,8)));return ac(s,i,r)}function rl(e){if(Array.isArray(e)){if(e.length<2)return;const t=Number(e[0]),i=Number(e[1]);return Number.isNaN(t)||Number.isNaN(i)?void 0:{x:t,y:i}}if(e&&typeof e=="object"){const t=e,i=Number(t.x??t.X??t.left),r=Number(t.y??t.Y??t.top);return Number.isNaN(i)||Number.isNaN(r)?void 0:{x:i,y:r}}if(typeof e=="string"&&e.trim())try{const t=JSON.parse(e);return rl(t)}catch{const t=e.split(/[,;\s]+/).map(i=>Number(i.trim()));if(t.length>=2&&!t.some(i=>Number.isNaN(i)))return{x:t[0],y:t[1]}}}function nh(e){return[Number(e.x.toFixed(6)),Number(e.y.toFixed(6))]}function Yv(e,t=2){return`${e.x.toFixed(t)}, ${e.y.toFixed(t)}`}function Wv(e,t){return{x:lc(e.x,t.step,t.xMin,t.xMax),y:lc(e.y,t.step,t.yMin,t.yMax)}}function cc(e,t,i,r){return{x:(e+t)/2,y:(i+r)/2}}var Kv=Object.defineProperty,Zv=Object.getOwnPropertyDescriptor,Te=(e,t,i,r)=>{for(var o=r>1?void 0:r?Zv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Kv(t,i,o),o};let de=class extends y{constructor(){super(...arguments),this.x=0,this.y=0,this.xMin=0,this.xMax=1,this.yMin=0,this.yMax=1,this.step=.01,this.invertY=!0,this.showGrid=!0,this.disabled=!1,this._pressed=!1,this._onPointerDown=e=>{var t;this.disabled||(e.preventDefault(),this._pressed=!0,this._pointerId=e.pointerId,(t=this._pad)==null||t.setPointerCapture(e.pointerId),this._setFromPointer(e.clientX,e.clientY))},this._onPointerMove=e=>{!this._pressed||e.pointerId!==this._pointerId||this._setFromPointer(e.clientX,e.clientY)},this._onPointerUp=e=>{var t;if(e.pointerId===this._pointerId){this._pressed=!1,this._pointerId=void 0;try{(t=this._pad)==null||t.releasePointerCapture(e.pointerId)}catch{}this.dispatchEvent(new CustomEvent("drag-end",{bubbles:!0,composed:!0,detail:{x:this.x,y:this.y}}))}},this._onKeyDown=e=>{if(this.disabled)return;let t=this.x,i=this.y;const r=this.step>0?this.step:.01;switch(e.key){case"ArrowRight":t+=r;break;case"ArrowLeft":t-=r;break;case"ArrowUp":i+=this.invertY?r:-r;break;case"ArrowDown":i+=this.invertY?-r:r;break;case"Home":t=this.xMin,i=this.yMin;break;case"End":t=this.xMax,i=this.yMax;break;default:return}e.preventDefault();const o=this._normalize({x:t,y:i});this.x=o.x,this.y=o.y,this._emitChange(o)}}_emitChange(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:e}))}_normalize(e){return Wv(e,{xMin:this.xMin,xMax:this.xMax,yMin:this.yMin,yMax:this.yMax,step:this.step})}_setFromPointer(e,t){const i=this._pad;if(!i)return;const r=i.getBoundingClientRect();if(r.width<=0||r.height<=0)return;const o=Math.max(0,Math.min(1,(e-r.left)/r.width)),n=Math.max(0,Math.min(1,(t-r.top)/r.height)),s=this.xMin+o*(this.xMax-this.xMin),a=this.invertY?1-n:n,l=this.yMin+a*(this.yMax-this.yMin),c=this._normalize({x:s,y:l});this.x=c.x,this.y=c.y,this._emitChange(c)}_handleStyle(){const e=this.xMax-this.xMin||1,t=this.yMax-this.yMin||1,i=(this.x-this.xMin)/e*100,r=(this.y-this.yMin)/t,o=(this.invertY?1-r:r)*100;return{left:`${i}%`,top:`${o}%`}}render(){return u`
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
        ${this.showGrid?u`
              <div class="grid" aria-hidden="true">
                <span class="hline"></span>
                <span class="vline"></span>
              </div>
            `:null}
        <div
          class="handle ${this._pressed?"pressed":""}"
          style=${T(this._handleStyle())}
          aria-hidden="true"
        ></div>
      </div>
    `}};de.styles=$`
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
  `;Te([p({type:Number})],de.prototype,"x",2);Te([p({type:Number})],de.prototype,"y",2);Te([p({type:Number,attribute:"x-min"})],de.prototype,"xMin",2);Te([p({type:Number,attribute:"x-max"})],de.prototype,"xMax",2);Te([p({type:Number,attribute:"y-min"})],de.prototype,"yMin",2);Te([p({type:Number,attribute:"y-max"})],de.prototype,"yMax",2);Te([p({type:Number})],de.prototype,"step",2);Te([p({type:Boolean,attribute:"invert-y"})],de.prototype,"invertY",2);Te([p({type:Boolean,attribute:"show-grid"})],de.prototype,"showGrid",2);Te([p({type:Boolean,reflect:!0})],de.prototype,"disabled",2);Te([b()],de.prototype,"_pressed",2);Te([zo(".pad")],de.prototype,"_pad",2);de=Te([w("ha-control-xy-pad")],de);var Qv=Object.defineProperty,Jv=Object.getOwnPropertyDescriptor,ps=(e,t,i,r)=>{for(var o=r>1?void 0:r?Jv(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Qv(t,i,o),o};function qr(e,t){if(e==null||e==="")return t;const i=Number(e);return Number.isNaN(i)?t:i}let Ti=class extends y{static getStubConfig(){return{type:"xy-pad",entity:"/demo/xy",name:"XY Pad",icon:"mdi:axis-arrow",x_min:le.x_min,x_max:le.x_max,y_min:le.y_min,y_max:le.y_max,step:le.step,invert_y:le.invert_y,show_grid:le.show_grid,snap_center:le.snap_center}}setConfig(e){this._config=e,this._local=void 0,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?4:3;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_ranges(){var e,t,i,r,o,n,s,a;return{xMin:qr((e=this._config)==null?void 0:e.x_min,le.x_min),xMax:qr((t=this._config)==null?void 0:t.x_max,le.x_max),yMin:qr((i=this._config)==null?void 0:i.y_min,le.y_min),yMax:qr((r=this._config)==null?void 0:r.y_max,le.y_max),step:qr((o=this._config)==null?void 0:o.step,le.step),invertY:((n=this._config)==null?void 0:n.invert_y)===void 0?le.invert_y:!!this._config.invert_y,showGrid:((s=this._config)==null?void 0:s.show_grid)===void 0?le.show_grid:!!this._config.show_grid,snapCenter:!!((a=this._config)!=null&&a.snap_center)}}_entityX(){var t;const e=(t=this._config)==null?void 0:t.entity_x;if(typeof e=="string"&&e.trim())return e.trim()}_entityY(){var t;const e=(t=this._config)==null?void 0:t.entity_y;if(typeof e=="string"&&e.trim())return e.trim()}_usesDualEntities(){return!!(this._entityX()&&this._entityY())}_readRemotePoint(){var s,a,l,c;const{xMin:e,xMax:t,yMin:i,yMax:r}=this._ranges(),o=cc(e,t,i,r);if(!this.flow)return o;if(this._usesDualEntities()){const d=this._entityX(),h=this._entityY(),f=(s=this.flow.states[d])==null?void 0:s.state,v=(a=this.flow.states[h])==null?void 0:a.state,g=typeof f=="number"?f:Number(f),_=typeof v=="number"?v:Number(v);return{x:Number.isNaN(g)?o.x:g,y:Number.isNaN(_)?o.y:_}}const n=(l=this._config)==null?void 0:l.entity;return n?rl((c=this.flow.states[n])==null?void 0:c.state)??o:o}_currentPoint(){return this._local??this._readRemotePoint()}async _writePoint(e){var i;if(!this.flow)return;if(this._usesDualEntities()){const r=this._entityX(),o=this._entityY();await Promise.all([this.flow.callService(r,e.x),this.flow.callService(o,e.y)]);return}const t=(i=this._config)==null?void 0:i.entity;t&&await this.flow.callService(t,nh(e))}async _onValueChanged(e){e.stopPropagation(),this._local=e.detail,await this._writePoint(e.detail)}async _onDragEnd(e){e.stopPropagation();const{snapCenter:t,xMin:i,xMax:r,yMin:o,yMax:n}=this._ranges();if(t){const s=cc(i,r,o,n);this._local=s,await this._writePoint(s)}window.setTimeout(()=>{this._local=void 0},250)}render(){var v,g;if(!this._config)return m;const e=this._ranges(),t=this._currentPoint(),r=this._usesDualEntities()?this._entityX():this._config.entity,o=r&&this.flow?this.flow.states[r]:void 0,n=!!this._config.vertical,s=String(this._config.name??((v=o==null?void 0:o.attributes)==null?void 0:v.friendly_name)??r??"XY Pad"),a=String(this._config.icon??((g=o==null?void 0:o.attributes)==null?void 0:g.icon)??"mdi:axis-arrow"),l=Yv(t),c=!0,d=Ni(c,this._config.color,"primary"),h=T({"--tile-color":d}),f=T({"--feature-color":d??"var(--primary-color)"});return u`
      <ha-card class=${O({active:c})} style=${h}>
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
              ?invert-y=${e.invertY}
              ?show-grid=${e.showGrid}
              @value-changed=${this._onValueChanged}
              @drag-end=${this._onDragEnd}
              @click=${_=>_.stopPropagation()}
            ></ha-control-xy-pad>
          </div>
        </ha-tile-container>
      </ha-card>
    `}};Ti.styles=[we,$e,$`
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
    `];ps([p({attribute:!1})],Ti.prototype,"flow",2);ps([b()],Ti.prototype,"_config",2);ps([b()],Ti.prototype,"_local",2);Ti=ps([w("hui-xy-pad-card")],Ti);const eb=new Set(["playing","play","on","true","running","started"]);function ol(e){if(!e)return!1;const t=e.state;return typeof t=="boolean"?t:typeof t=="number"?t>0:eb.has(String(t).toLowerCase())}function tb(e){return ol(e)?"play":"stop"}function ib(e){return ol(e)?"正在播放":"已停止"}function rb(e,t){const i=e==="play"?"play_value":"stop_value";return i in(t??{})?ob(t==null?void 0:t[i]):e==="play"}function ob(e){if(typeof e=="boolean"||typeof e=="number")return e;if(e==null||e==="")return!0;const t=String(e).trim();if(t==="true")return!0;if(t==="false")return!1;const i=Number(t);return!Number.isNaN(i)&&t!==""?i:t}var nb=Object.defineProperty,sb=Object.getOwnPropertyDescriptor,nl=(e,t,i,r)=>{for(var o=r>1?void 0:r?sb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&nb(t,i,o),o};let ko=class extends y{constructor(){super(...arguments),this.value="stop",this.disabled=!1}_emit(e){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{action:e}}))}_select(e,t){t.stopPropagation(),!(this.disabled||this.value===e)&&(this.value=e,this._emit(e))}render(){return u`
      <div class="container" role="radiogroup" aria-label="播放控制">
        <button
          type="button"
          class=${O({segment:!0,selected:this.value==="play"})}
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
          class=${O({segment:!0,selected:this.value==="stop"})}
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
    `}};ko.styles=$`
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
  `;nl([p({type:String})],ko.prototype,"value",2);nl([p({type:Boolean})],ko.prototype,"disabled",2);ko=nl([w("ha-control-media-playback")],ko);var ab=Object.defineProperty,lb=Object.getOwnPropertyDescriptor,sl=(e,t,i,r)=>{for(var o=r>1?void 0:r?lb(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ab(t,i,o),o};let br=class extends y{static getStubConfig(){return{type:"media",entity:"/demo/media",name:"媒体播放器",icon:"mdi:cast-audio"}}setConfig(e){this._config=e,this.requestUpdate()}getGridOptions(){var i;const e=!!((i=this._config)!=null&&i.vertical),t=e?3:2;return{columns:6,rows:t,min_columns:e?3:6,min_rows:t}}_entityState(){var t;const e=(t=this._config)==null?void 0:t.entity;if(!(!e||!this.flow))return this.flow.states[e]}async _onPlaybackChange(e){var r;e.stopPropagation();const t=(r=this._config)==null?void 0:r.entity;if(!t||!this.flow)return;const i=rb(e.detail.action,this._config);await this.flow.callService(t,i)}render(){var d,h;if(!this._config)return m;const e=this._config.entity,t=this._entityState(),i=ol(t),r=tb(t),o=!!this._config.vertical,n=String(this._config.name??((d=t==null?void 0:t.attributes)==null?void 0:d.friendly_name)??e??"媒体播放器"),s=String(this._config.icon??((h=t==null?void 0:t.attributes)==null?void 0:h.icon)??"mdi:cast-audio"),a=Ni(i,this._config.color,"primary"),l=T({"--tile-color":a}),c=T({"--feature-color":a??(i?"var(--primary-color)":"var(--state-inactive-color)")});return u`
      <ha-card class=${O({active:i})} style=${l}>
        <ha-tile-container ?vertical=${o} ?fixed-info-height=${o}>
          <ha-tile-icon slot="icon" .icon=${s} .active=${i}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${n}
            .secondary=${ib(t)}
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
    `}};br.styles=[we,$e,$`
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
    `];sl([p({attribute:!1})],br.prototype,"flow",2);sl([b()],br.prototype,"_config",2);br=sl([w("hui-media-card")],br);var cb=Object.defineProperty,db=Object.getOwnPropertyDescriptor,al=(e,t,i,r)=>{for(var o=r>1?void 0:r?db(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&cb(t,i,o),o};const hb=new Set(["on","true","1","ok","normal","connected","open","playing","ready","正常"]),ub=new Set(["off","false","0","error","fail","failed","disconnected","closed","unavailable","异常","错误","断开"]);function dc(e){return Array.isArray(e)?e.map(t=>{if(!t||typeof t!="object")return null;const i=t,r=String(i.entity??"").trim();if(!r)return null;const o=i.name!=null?String(i.name):void 0,n=i.icon!=null?String(i.icon):void 0;return{entity:r,...o?{name:o}:{},...n?{icon:n}:{}}}).filter(t=>t!==null):[]}function pb(e){if(!e||e.state===null||e.state===void 0||e.state==="")return"unknown";const t=e.state;if(typeof t=="boolean")return t?"ok":"error";if(typeof t=="number")return t>0?"ok":"error";if(Array.isArray(t))return t.length>0?"ok":"error";const i=String(t).toLowerCase().trim();return hb.has(i)?"ok":ub.has(i)?"error":Re(e)?"ok":"error"}function fb(e){return e==="ok"?"正常":e==="error"?"异常":"—"}let _r=class extends y{constructor(){super(...arguments),this._unsubs=[],this._subscribedKey=""}static getStubConfig(){return{type:"status",name:"Flow 连接",icon:"mdi:lan-connect",color:"green",entities:[{entity:"/demo/status_ws",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_tcp",name:"tcp连接",icon:"mdi:lan-connect"},{entity:"/demo/status_udp",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_osc",name:"connected",icon:"mdi:lan-connect"}]}}setConfig(e){this._config=e,this._syncSubscriptions(),this.requestUpdate()}connectedCallback(){super.connectedCallback(),this._syncSubscriptions()}disconnectedCallback(){this._clearSubscriptions(),super.disconnectedCallback()}updated(e){(e.has("flow")||e.has("_config"))&&this._syncSubscriptions()}_clearSubscriptions(){this._unsubs.forEach(e=>e()),this._unsubs=[],this._subscribedKey=""}_syncSubscriptions(){var r;const e=this.flow,t=dc((r=this._config)==null?void 0:r.entities),i=`${e?"1":"0"}:${t.map(o=>o.entity).join("\0")}`;i===this._subscribedKey&&this._unsubs.length>0||(this._clearSubscriptions(),e&&(this._subscribedKey=i,t.forEach(o=>{this._unsubs.push(e.subscribeEntity(o.entity,()=>{this.requestUpdate()}))})))}getGridOptions(){var t;const e=!!((t=this._config)!=null&&t.vertical);return{columns:e?3:6,rows:"auto",min_columns:e?3:6,min_rows:2}}render(){var _;if(!this._config)return m;(_=this.flow)==null||_.statesRevision;const e=dc(this._config.entities),t=String(this._config.name??"状态"),i=String(this._config.icon??"mdi:lan-connect"),r=!!this._config.vertical,o=Number(this._config.list_columns)===2?2:1,n=Ta(this._config.color)??"var(--green-color, #4caf50)",s=e.map(k=>{var S;return pb((S=this.flow)==null?void 0:S.states[k.entity])}),a=s.filter(k=>k==="ok").length,l=e.length,c=l>0&&a===l,d=s.some(k=>k==="error"),h=l===0?"unknown":c?"ok":d?"error":"unknown",f=l===0?"无绑定":`${a}/${l} ${h==="ok"?"正常":h==="error"?"异常":"未知"}`,v=h==="error"?"var(--red-color, #f44336)":h==="unknown"?"var(--state-inactive-color, #9e9e9e)":n,g=T({"--status-ok":n,"--status-accent":v,"--tile-color":v});return u`
      <ha-card
        class=${O({vertical:r,ok:h==="ok",error:h==="error",unknown:h==="unknown"})}
        style=${g}
      >
        <ha-tile-container ?vertical=${r} expand-features>
          <ha-tile-icon slot="icon" .icon=${i}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${t}
            .secondary=${f}
          ></ha-tile-info>
          <div slot="features" class=${O({list:!0,"cols-2":o===2})}>
            ${e.length===0?u`<p class="empty">添加地址后显示各项状态</p>`:e.map((k,S)=>{var B,J,ae,q,De;const M=(B=this.flow)==null?void 0:B.states[k.entity],N=s[S]??"unknown",R=((J=k.name)==null?void 0:J.trim())||String(((ae=M==null?void 0:M.attributes)==null?void 0:ae.friendly_name)??"")||k.entity,Q=((q=k.icon)==null?void 0:q.trim())||String(((De=M==null?void 0:M.attributes)==null?void 0:De.icon)??"")||i;return u`
                    <div
                      class=${O({row:!0,ok:N==="ok",error:N==="error",unknown:N==="unknown"})}
                    >
                      <ha-icon class="row-icon" .icon=${Q}></ha-icon>
                      <span class="row-name">${R}</span>
                      <span class="row-status">
                        <span class="dot"></span>
                        <span class="status-text">${fb(N)}</span>
                      </span>
                    </div>
                  `})}
          </div>
        </ha-tile-container>
      </ha-card>
    `}};_r.styles=$`
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
  `;al([p({attribute:!1})],_r.prototype,"flow",2);al([b()],_r.prototype,"_config",2);_r=al([w("hui-status-card")],_r);function Gr(e,t,i,r,o,n){const s={type:e,name:t,description:i,icon:r,category:"sensor",lazy:!0,loader:async()=>{const a=await o();s.constructor=a[n]}};G(s)}function gb(){G({type:"heading",name:"标题",description:"分区标题行（含状态徽章与控制徽章）",icon:"mdi:format-title",category:"container",constructor:nr}),G({type:"tile",name:"磁贴",description:"开关切换磁贴",icon:"mdi:view-grid",category:"tile",constructor:ur}),G({type:"sensor",name:"数值",description:"显示地址数值",icon:"mdi:gauge",category:"sensor",constructor:lr}),G({type:"label",name:"标签",description:"只读显示字符串（设备名、素材名、错误信息等）",icon:"mdi:label-outline",category:"sensor",constructor:cr}),G({type:"status",name:"状态",description:"多地址状态列表（汇总正常/异常）",icon:"mdi:lan-connect",category:"sensor",constructor:_r}),G({type:"clock",name:"时钟",description:"数字/模拟时钟",icon:"mdi:clock-outline",category:"container",constructor:pr}),G({type:"climate",name:"步进",description:"步进磁贴（目标值 +/-）",icon:"mdi:home-thermometer",category:"control",constructor:fr}),G({type:"cover",name:"单选",description:"单选磁贴（位置分段）",icon:"mdi:window-shutter",category:"control",constructor:mr}),G({type:"trigger",name:"按钮",description:"触发器（收到 true 时短暂亮起）",icon:"mdi:gesture-tap-button",category:"control",constructor:ki}),G({type:"switch",name:"开关",description:"开关磁贴（Flood light 风格）",icon:"mdi:toggle-switch",category:"control",constructor:hr}),G({type:"slider",name:"滑块",description:"亮度滑块（Spotlights 风格）",icon:"mdi:tune-vertical",category:"control",constructor:dr}),G({type:"gain",name:"增益",description:"增益调节（dB 步进，点击静音键降至最小值并锁定）",icon:"mdi:volume-high",category:"control",constructor:Ci}),G({type:"rgba",name:"RGBA",description:"RGBA 颜色选择（绑定 [r,g,b,a] 0–1 地址）",icon:"mdi:palette",category:"control",constructor:$o}),G({type:"hsv",name:"HSV",description:"HSV 颜色选择（绑定 [h,s,v] 0–1 地址）",icon:"mdi:palette-swatch",category:"control",constructor:ra}),G({type:"color",name:"颜色（旧）",description:"已更名为 RGBA，保留兼容",icon:"mdi:palette",category:"control",constructor:ia}),G({type:"multi-fader",name:"多维推杆",description:"多个推杆，输出 N 维数组到同一地址",icon:"mdi:tune-vertical-variant",category:"control",constructor:Pi}),G({type:"xy-pad",name:"XY 触控板",description:"二维触控板（绑定 [x,y] 或分别绑定 X/Y 地址）",icon:"mdi:axis-arrow",category:"control",constructor:Ti}),G({type:"media",name:"媒体",description:"播放器启停控制（播放 / 停止）",icon:"mdi:cast-audio",category:"control",constructor:br}),G({type:"markdown",name:"Markdown",description:"文本说明卡片",icon:"mdi:text",category:"container",constructor:ar}),G({type:"link",name:"超链接",description:"点击跳转到外部或内部链接",icon:"mdi:link-variant",category:"container",constructor:vr}),G({type:"picture-elements",name:"图片元素",description:"在底图上叠加状态徽章与控制徽章",icon:"mdi:floor-plan",category:"container",constructor:Tt}),Gr("line3d","3D 折线","实时三维折线图（订阅 OSC 地址推送的点坐标）","mdi:chart-timeline-variant",()=>Wi(()=>import("./hui-line3d-card-C-h9W1E5.js"),__vite__mapDeps([0,1,2,3]),import.meta.url),"HuiLine3dCard"),Gr("scatter3d","3D 散点","实时三维散点图（订阅 OSC 地址推送的点坐标）","mdi:chart-scatter-plot",()=>Wi(()=>import("./hui-scatter3d-card-DjuYvItv.js"),__vite__mapDeps([4,1,2,3]),import.meta.url),"HuiScatter3dCard"),Gr("line2d","2D 折线","实时二维折线图（订阅 OSC 地址推送的 [x,y] 坐标）","mdi:chart-line",()=>Wi(()=>import("./hui-line2d-card-DMoeVuwq.js"),__vite__mapDeps([5,6,2,3]),import.meta.url),"HuiLine2dCard"),Gr("scatter2d","2D 散点","实时二维散点图（订阅 OSC 地址推送的 [x,y] 坐标）","mdi:chart-scatter-plot-hexbin",()=>Wi(()=>import("./hui-scatter2d-card-BLtS0gsb.js"),__vite__mapDeps([7,6,2,3]),import.meta.url),"HuiScatter2dCard"),Gr("bar","柱状图","每根柱绑定一个地址，显示该地址的实时数值","mdi:chart-bar",()=>Wi(()=>import("./hui-bar-card-CMEZKviK.js"),__vite__mapDeps([8,6,2,3]),import.meta.url),"HuiBarCard"),G({type:"error",name:"Error",category:"container",constructor:vo})}gb();let Wr=null,Kr=null;function hc(e){return new Promise(t=>{Wr=t,document.dispatchEvent(new CustomEvent("flow-show-confirm",{bubbles:!0,composed:!0,detail:e}))})}function mb(e){Wr==null||Wr(e),Wr=null}function vb(e){return new Promise(t=>{Kr=t,document.dispatchEvent(new CustomEvent("flow-show-delete-card",{bubbles:!0,composed:!0,detail:{cardConfig:e}}))})}function bb(e){Kr==null||Kr(e),Kr=null}function sh(e,t,i){const r=[...e],[o]=r.splice(t,1);return r.splice(i,0,o),r}function _b(e,t){return e.views[t]}function Ut(e,t,i){e.sections=e.sections.map((r,o)=>o===t?i:r)}function yb(e,t){e.sections=[...e.sections,t]}function xb(e,t){return{...e,cards:[...e.cards,t]}}function wb(e){if(Array.isArray(e)&&e.length===3){const[t,i,r]=e;if(typeof t=="number"&&typeof i=="number"&&typeof r=="number")return{viewIndex:t,sectionIndex:i,cardIndex:r}}if(e&&typeof e=="object"){const t=e;if(typeof t.viewIndex=="number"&&typeof t.sectionIndex=="number"&&typeof t.cardIndex=="number")return t}}function uc(e,t,i,r,o){const n=e.sections[t],s=e.sections[r];if(!n||!s)return;const a=n.cards[i];if(!a)return;if(t===r){Ut(e,t,{...n,cards:sh(n.cards,i,o)});return}const l=n.cards.filter((h,f)=>f!==i),c=[...s.cards],d=Math.max(0,Math.min(o,c.length));c.splice(d,0,a),Ut(e,t,{...n,cards:l}),Ut(e,r,{...s,cards:c})}function $b(e,t,i){e.sections=sh(e.sections,t,i)}function kb(e,t){const i=e.cards[t];if(!i)return e;const r=structuredClone(i),o=[...e.cards];return o.splice(t+1,0,r),{...e,cards:o}}function Sb(e,t){return{...e,cards:e.cards.filter((i,r)=>r!==t)}}function Cb(e,t){const i=e.sections[t];if(!i)return;const r=structuredClone(i),o=[...e.sections];o.splice(t+1,0,r),e.sections=o}function Eb(e,t){e.sections=e.sections.filter((i,r)=>r!==t)}function Pb(e,t,i){if(!e.cards[t])return e;const o=e.cards.map((n,s)=>s===t?{...i}:n);return{...e,cards:o}}function Tb(e,t,i){const r=e.sections[t];if(!r)return;const o={...r,...i};i.background===void 0&&"background"in i&&delete o.background,Ut(e,t,o)}function pc(e,t){const i=e.views;if(!i.length||t===void 0||t==="")return 0;if(typeof t=="number")return Math.max(0,Math.min(t,i.length-1));const r=Number(t);for(let o=0;o<i.length;o++)if(i[o].path===t||o===r)return o;return 0}function na(e,t){return e.path||String(t)}function Ob(e,t,i){const r=[...e.views],o=r.length;return r.splice(o,0,t),e.views=r,o}function Ab(e,t){return e.views.length<=1?0:(e.views=e.views.filter((i,r)=>r!==t),Math.min(t,e.views.length-1))}function Db(e,t,i){e.views[t]&&(e.views=e.views.map((o,n)=>n===t?{...o,...i}:o))}const Ib=`欢迎使用 **Flow Dashboard**。

布局采用 Section + Card 结构，仿照 [Home Assistant Sections](https://www.home-assistant.io/dashboards/sections/)。

- 支持 **粗体** / *斜体*
- 支持列表与链接
- 可在编辑器中切换卡片 / 纯文本样式`;function Mb(){return md({title:"Flow",views:[{title:"Home",path:"home",icon:"mdi:home",max_columns:4,sections:[{type:"grid",column_span:4,cards:[{type:"heading",heading:"Welcome",icon:"mdi:hand-wave",badges:[{type:"entity",entity:"/demo/temperature",icon:"mdi:thermometer"},{type:"entity",entity:"/demo/humidity",icon:"mdi:water-percent"}]},{type:"markdown",content:Ib}]},{type:"grid",column_span:1,cards:[{type:"clock",clock_style:"analog",clock_size:"small",time_format:"24",grid_options:{columns:6,rows:2,min_columns:3,min_rows:2}},{type:"media",entity:"/demo/media",name:"媒体播放器",icon:"mdi:cast-audio"},{type:"slider",entity:"/demo/spotlights",name:"Spotlights",icon:"mdi:spotlight-beam"},{type:"rgba",entity:"/demo/rgba",name:"RGBA",icon:"mdi:palette",grid_options:{columns:6,rows:5,min_columns:6,min_rows:5}},{type:"hsv",entity:"/demo/hsv",name:"HSV",icon:"mdi:palette-swatch",grid_options:{columns:6,rows:5,min_columns:6,min_rows:5}},{type:"multi-fader",entity:"/demo/faders",name:"多维推杆",icon:"mdi:tune-vertical-variant",count:4,min:0,max:1,step:.01,labels:"0,1,2,3",orientation:"vertical"},{type:"xy-pad",entity:"/demo/xy",name:"XY Pad",icon:"mdi:axis-arrow"},{type:"climate",entity:"/demo/climate",name:"Upstairs",icon:"mdi:home-thermometer",min:0,max:100,step:1,unit:"°C"},{type:"sensor",entity:"/demo/temperature",name:"Temperature",icon:"mdi:thermometer",unit:"°C"},{type:"label",entity:"/demo/string",name:"当前素材",icon:"mdi:filmstrip"},{type:"status",name:"Flow 连接",icon:"mdi:lan-connect",color:"green",entities:[{entity:"/demo/status_ws",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_tcp",name:"tcp连接",icon:"mdi:lan-connect"},{entity:"/demo/status_udp",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_osc",name:"connected",icon:"mdi:lan-connect"}],grid_options:{columns:6,rows:"auto",min_columns:3}}]},{type:"grid",column_span:1,cards:[{type:"switch",entity:"/demo/floor_lamp",name:"Flood light",icon:"mdi:lightbulb"},{type:"cover",entity:"/demo/kitchen_shutter",name:"Kitchen shutter",icon:"mdi:window-shutter",position_count:4},{type:"trigger",entity:"/demo/trigger",name:"Trigger",icon:"mdi:gesture-tap-button"}]},{type:"grid",column_span:1,cards:[{type:"gain",entity:"/demo/gain",name:"输入增益",icon:"mdi:volume-high",min:-60,max:12,step:1}]}]},{title:"设备",path:"devices",icon:"mdi:devices",max_columns:4,sections:[{type:"grid",column_span:2,cards:[{type:"heading",heading:"连接状态",icon:"mdi:lan-connect"},{type:"status",name:"Flow 连接",icon:"mdi:lan-connect",color:"green",list_columns:2,entities:[{entity:"/demo/status_ws",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_tcp",name:"tcp连接",icon:"mdi:lan-connect"},{entity:"/demo/status_udp",name:"connected",icon:"mdi:lan-connect"},{entity:"/demo/status_osc",name:"connected",icon:"mdi:lan-connect"}]},{type:"tile",entity:"/demo/lamp",name:"台灯",icon:"mdi:desk-lamp"},{type:"switch",entity:"/demo/switch",name:"开关",icon:"mdi:toggle-switch"}]}]}]})}const nn=Mb();function zb(e){return e.version===2?!Array.isArray(e.views)||e.views.length===0?!0:e.views.every(t=>{const i=t.sections??[];return i.length===0?!0:i.every(r=>{var o;return(((o=r.cards)==null?void 0:o.length)??0)===0})}):Array.isArray(e.items)?e.items.length===0:!0}async function Nb(){try{const e=await fetch("/api/layout/load");if(!e.ok)return nn;const t=await e.json();return zb(t)?nn:t.version===2&&Array.isArray(t.views)?md({title:"Flow",views:t.views}):nn}catch{return nn}}async function Rb(e){await fetch("/api/layout/save",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({version:2,...e})})}function ah(e){const t=new Set;return e.views.forEach(i=>{i.sections.forEach(r=>{r.cards.forEach(o=>{o.entity&&t.add(o.entity);const n=o.entity_x;typeof n=="string"&&n&&t.add(n);const s=o.entity_y;typeof s=="string"&&s&&t.add(s);const a=o.badges;a==null||a.forEach(d=>{d.entity&&t.add(d.entity)});const l=o.elements;l==null||l.forEach(d=>{d.entity&&t.add(d.entity)});const c=o.entities;c==null||c.forEach(d=>{d.entity&&t.add(d.entity)})})})}),[...t]}const lh="flow_layout_edit_enabled",sa="flow-layout-edit-enabled-changed";function ch(){try{return localStorage.getItem(lh)==="true"}catch{return!1}}function Lb(e){try{localStorage.setItem(lh,e?"true":"false")}catch{}document.dispatchEvent(new CustomEvent(sa,{detail:{enabled:e}}))}const dh="dashboardCardClipboard",hh="dashboardRecentCardTypes",Bb=4,aa="flow-dashboard-card-clipboard-changed";function Fb(){try{const e=sessionStorage.getItem(dh);if(!e)return;const t=JSON.parse(e);return!t||typeof t!="object"||!t.type?void 0:t}catch{return}}function Hb(e){try{sessionStorage.setItem(dh,JSON.stringify(e))}catch{}document.dispatchEvent(new CustomEvent(aa))}function Ub(e){return document.addEventListener(aa,e),window.addEventListener("storage",e),()=>{document.removeEventListener(aa,e),window.removeEventListener("storage",e)}}function uh(){try{const e=localStorage.getItem(hh);if(!e)return[];const t=JSON.parse(e);return Array.isArray(t)?t.filter(i=>typeof i=="string"&&!!i):[]}catch{return[]}}function jb(e){if(!e||e==="error"||e==="color")return;const t=[e,...uh().filter(i=>i!==e)].slice(0,Bb);try{localStorage.setItem(hh,JSON.stringify(t))}catch{}}const Vb=["heading","tile","sensor","status"],fc=4,qb=75;class Gb{constructor(t){this._undoStack=[],this._redoStack=[],this._stackLimit=t.stackLimit??qb,this._currentConfig=t.currentConfig,this._apply=t.apply,this._onStackChange=t.onStackChange}get canUndo(){return this._undoStack.length>0}get canRedo(){return this._redoStack.length>0}commit(t){var i;this._undoStack.length>=this._stackLimit&&this._undoStack.shift(),this._undoStack.push(t),this._redoStack=[],(i=this._onStackChange)==null||i.call(this)}undo(){var i;if(this._undoStack.length===0)return;this._redoStack.push(this._currentConfig());const t=this._undoStack.pop();this._apply(t),(i=this._onStackChange)==null||i.call(this)}redo(){var i;if(this._redoStack.length===0)return;this._undoStack.push(this._currentConfig());const t=this._redoStack.pop();this._apply(t),(i=this._onStackChange)==null||i.call(this)}reset(){var t;this._undoStack=[],this._redoStack=[],(t=this._onStackChange)==null||t.call(this)}}function Xb(e,t){const i=window.matchMedia(e),r=o=>t(o.matches);return i.addEventListener("change",r),t(i.matches),()=>i.removeEventListener("change",r)}/**
 * @license
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const{I:Yb}=zp,gc=e=>e,mc=()=>document.createComment(""),Xr=(e,t,i)=>{var n;const r=e._$AA.parentNode,o=t===void 0?e._$AB:t._$AA;if(i===void 0){const s=r.insertBefore(mc(),o),a=r.insertBefore(mc(),o);i=new Yb(s,a,e,e.options)}else{const s=i._$AB.nextSibling,a=i._$AM,l=a!==e;if(l){let c;(n=i._$AQ)==null||n.call(i,e),i._$AM=e,i._$AP!==void 0&&(c=e._$AU)!==a._$AU&&i._$AP(c)}if(s!==o||l){let c=i._$AA;for(;c!==s;){const d=gc(c).nextSibling;gc(r).insertBefore(c,o),c=d}}}return i},ai=(e,t,i=e)=>(e._$AI(t,i),e),Wb={},Kb=(e,t=Wb)=>e._$AH=t,Zb=e=>e._$AH,Ds=e=>{e._$AR(),e._$AA.remove()};/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */const vc=(e,t,i)=>{const r=new Map;for(let o=t;o<=i;o++)r.set(e[o],o);return r},ph=Ea(class extends Pa{constructor(e){if(super(e),e.type!==Ca.CHILD)throw Error("repeat() can only be used in text expressions")}dt(e,t,i){let r;i===void 0?i=t:t!==void 0&&(r=t);const o=[],n=[];let s=0;for(const a of e)o[s]=r?r(a,s):s,n[s]=i(a,s),s++;return{values:n,keys:o}}render(e,t,i){return this.dt(e,t,i).values}update(e,[t,i,r]){const o=Zb(e),{values:n,keys:s}=this.dt(t,i,r);if(!Array.isArray(o))return this.ut=s,n;const a=this.ut??(this.ut=[]),l=[];let c,d,h=0,f=o.length-1,v=0,g=n.length-1;for(;h<=f&&v<=g;)if(o[h]===null)h++;else if(o[f]===null)f--;else if(a[h]===s[v])l[v]=ai(o[h],n[v]),h++,v++;else if(a[f]===s[g])l[g]=ai(o[f],n[g]),f--,g--;else if(a[h]===s[g])l[g]=ai(o[h],n[g]),Xr(e,l[g+1],o[h]),h++,g--;else if(a[f]===s[v])l[v]=ai(o[f],n[v]),Xr(e,o[h],o[f]),f--,v++;else if(c===void 0&&(c=vc(s,v,g),d=vc(a,h,f)),c.has(a[h]))if(c.has(a[f])){const _=d.get(s[v]),k=_!==void 0?o[_]:null;if(k===null){const S=Xr(e,o[h]);ai(S,n[v]),l[v]=S}else l[v]=ai(k,n[v]),Xr(e,o[h],k),o[_]=null;v++}else Ds(o[f]),f--;else Ds(o[h]),h++;for(;v<=g;){const _=Xr(e,l[g+1]);ai(_,n[v]),l[v++]=_}for(;h<=f;){const _=o[h++];_!==null&&Ds(_)}return this.ut=s,Kb(e,l),Et}}),fh=4;function sn(e){return parseInt(e.replace("px","").trim(),10)||0}function Qb(e){return e.reduce((t,i)=>t+(i.column_span??1),0)}function Jb(e,t={}){const i=t.minColumnWidth??320,r=t.columnGap??32,o=t.horizontalPadding??0,n=t.configMaxColumns??fh,s=Math.floor((e-o+r)/(i+r));return Math.max(1,Math.min(n,s))}function e_(e){const{sectionColumnSpanSum:t,maxColumns:i,editMode:r=!1,extraSlots:o=0}=e,n=t+(r?1:0)+o;return Math.max(Math.min(i,n),1)}function t_(e,t){return Math.min(e??1,t)}/**!
 * Sortable 1.15.7
 * @author	RubaXa   <trash@rubaxa.org>
 * @author	owenm    <owen23355@gmail.com>
 * @license MIT
 */function i_(e,t,i){return(t=s_(t))in e?Object.defineProperty(e,t,{value:i,enumerable:!0,configurable:!0,writable:!0}):e[t]=i,e}function Ot(){return Ot=Object.assign?Object.assign.bind():function(e){for(var t=1;t<arguments.length;t++){var i=arguments[t];for(var r in i)({}).hasOwnProperty.call(i,r)&&(e[r]=i[r])}return e},Ot.apply(null,arguments)}function bc(e,t){var i=Object.keys(e);if(Object.getOwnPropertySymbols){var r=Object.getOwnPropertySymbols(e);t&&(r=r.filter(function(o){return Object.getOwnPropertyDescriptor(e,o).enumerable})),i.push.apply(i,r)}return i}function ut(e){for(var t=1;t<arguments.length;t++){var i=arguments[t]!=null?arguments[t]:{};t%2?bc(Object(i),!0).forEach(function(r){i_(e,r,i[r])}):Object.getOwnPropertyDescriptors?Object.defineProperties(e,Object.getOwnPropertyDescriptors(i)):bc(Object(i)).forEach(function(r){Object.defineProperty(e,r,Object.getOwnPropertyDescriptor(i,r))})}return e}function r_(e,t){if(e==null)return{};var i,r,o=o_(e,t);if(Object.getOwnPropertySymbols){var n=Object.getOwnPropertySymbols(e);for(r=0;r<n.length;r++)i=n[r],t.indexOf(i)===-1&&{}.propertyIsEnumerable.call(e,i)&&(o[i]=e[i])}return o}function o_(e,t){if(e==null)return{};var i={};for(var r in e)if({}.hasOwnProperty.call(e,r)){if(t.indexOf(r)!==-1)continue;i[r]=e[r]}return i}function n_(e,t){if(typeof e!="object"||!e)return e;var i=e[Symbol.toPrimitive];if(i!==void 0){var r=i.call(e,t);if(typeof r!="object")return r;throw new TypeError("@@toPrimitive must return a primitive value.")}return(t==="string"?String:Number)(e)}function s_(e){var t=n_(e,"string");return typeof t=="symbol"?t:t+""}function la(e){"@babel/helpers - typeof";return la=typeof Symbol=="function"&&typeof Symbol.iterator=="symbol"?function(t){return typeof t}:function(t){return t&&typeof Symbol=="function"&&t.constructor===Symbol&&t!==Symbol.prototype?"symbol":typeof t},la(e)}var a_="1.15.7";function $t(e){if(typeof window<"u"&&window.navigator)return!!navigator.userAgent.match(e)}var Mt=$t(/(?:Trident.*rv[ :]?11\.|msie|iemobile|Windows Phone)/i),jo=$t(/Edge/i),_c=$t(/firefox/i),ro=$t(/safari/i)&&!$t(/chrome/i)&&!$t(/android/i),ll=$t(/iP(ad|od|hone)/i),gh=$t(/chrome/i)&&$t(/android/i),mh={capture:!1,passive:!1};function I(e,t,i){e.addEventListener(t,i,!Mt&&mh)}function D(e,t,i){e.removeEventListener(t,i,!Mt&&mh)}function Bn(e,t){if(t){if(t[0]===">"&&(t=t.substring(1)),e)try{if(e.matches)return e.matches(t);if(e.msMatchesSelector)return e.msMatchesSelector(t);if(e.webkitMatchesSelector)return e.webkitMatchesSelector(t)}catch{return!1}return!1}}function vh(e){return e.host&&e!==document&&e.host.nodeType&&e.host!==e?e.host:e.parentNode}function Ye(e,t,i,r){if(e){i=i||document;do{if(t!=null&&(t[0]===">"?e.parentNode===i&&Bn(e,t):Bn(e,t))||r&&e===i)return e;if(e===i)break}while(e=vh(e))}return null}var yc=/\s+/g;function ke(e,t,i){if(e&&t)if(e.classList)e.classList[i?"add":"remove"](t);else{var r=(" "+e.className+" ").replace(yc," ").replace(" "+t+" "," ");e.className=(r+(i?" "+t:"")).replace(yc," ")}}function C(e,t,i){var r=e&&e.style;if(r){if(i===void 0)return document.defaultView&&document.defaultView.getComputedStyle?i=document.defaultView.getComputedStyle(e,""):e.currentStyle&&(i=e.currentStyle),t===void 0?i:i[t];!(t in r)&&t.indexOf("webkit")===-1&&(t="-webkit-"+t),r[t]=i+(typeof i=="string"?"":"px")}}function tr(e,t){var i="";if(typeof e=="string")i=e;else do{var r=C(e,"transform");r&&r!=="none"&&(i=r+" "+i)}while(!t&&(e=e.parentNode));var o=window.DOMMatrix||window.WebKitCSSMatrix||window.CSSMatrix||window.MSCSSMatrix;return o&&new o(i)}function bh(e,t,i){if(e){var r=e.getElementsByTagName(t),o=0,n=r.length;if(i)for(;o<n;o++)i(r[o],o);return r}return[]}function at(){var e=document.scrollingElement;return e||document.documentElement}function te(e,t,i,r,o){if(!(!e.getBoundingClientRect&&e!==window)){var n,s,a,l,c,d,h;if(e!==window&&e.parentNode&&e!==at()?(n=e.getBoundingClientRect(),s=n.top,a=n.left,l=n.bottom,c=n.right,d=n.height,h=n.width):(s=0,a=0,l=window.innerHeight,c=window.innerWidth,d=window.innerHeight,h=window.innerWidth),(t||i)&&e!==window&&(o=o||e.parentNode,!Mt))do if(o&&o.getBoundingClientRect&&(C(o,"transform")!=="none"||i&&C(o,"position")!=="static")){var f=o.getBoundingClientRect();s-=f.top+parseInt(C(o,"border-top-width")),a-=f.left+parseInt(C(o,"border-left-width")),l=s+n.height,c=a+n.width;break}while(o=o.parentNode);if(r&&e!==window){var v=tr(o||e),g=v&&v.a,_=v&&v.d;v&&(s/=_,a/=g,h/=g,d/=_,l=s+d,c=a+h)}return{top:s,left:a,bottom:l,right:c,width:h,height:d}}}function xc(e,t,i){for(var r=jt(e,!0),o=te(e)[t];r;){var n=te(r)[i],s=void 0;if(s=o>=n,!s)return r;if(r===at())break;r=jt(r,!1)}return!1}function yr(e,t,i,r){for(var o=0,n=0,s=e.children;n<s.length;){if(s[n].style.display!=="none"&&s[n]!==E.ghost&&(r||s[n]!==E.dragged)&&Ye(s[n],i.draggable,e,!1)){if(o===t)return s[n];o++}n++}return null}function cl(e,t){for(var i=e.lastElementChild;i&&(i===E.ghost||C(i,"display")==="none"||t&&!Bn(i,t));)i=i.previousElementSibling;return i||null}function ze(e,t){var i=0;if(!e||!e.parentNode)return-1;for(;e=e.previousElementSibling;)e.nodeName.toUpperCase()!=="TEMPLATE"&&e!==E.clone&&(!t||Bn(e,t))&&i++;return i}function wc(e){var t=0,i=0,r=at();if(e)do{var o=tr(e),n=o.a,s=o.d;t+=e.scrollLeft*n,i+=e.scrollTop*s}while(e!==r&&(e=e.parentNode));return[t,i]}function l_(e,t){for(var i in e)if(e.hasOwnProperty(i)){for(var r in t)if(t.hasOwnProperty(r)&&t[r]===e[i][r])return Number(i)}return-1}function jt(e,t){if(!e||!e.getBoundingClientRect)return at();var i=e,r=!1;do if(i.clientWidth<i.scrollWidth||i.clientHeight<i.scrollHeight){var o=C(i);if(i.clientWidth<i.scrollWidth&&(o.overflowX=="auto"||o.overflowX=="scroll")||i.clientHeight<i.scrollHeight&&(o.overflowY=="auto"||o.overflowY=="scroll")){if(!i.getBoundingClientRect||i===document.body)return at();if(r||t)return i;r=!0}}while(i=i.parentNode);return at()}function c_(e,t){if(e&&t)for(var i in t)t.hasOwnProperty(i)&&(e[i]=t[i]);return e}function Is(e,t){return Math.round(e.top)===Math.round(t.top)&&Math.round(e.left)===Math.round(t.left)&&Math.round(e.height)===Math.round(t.height)&&Math.round(e.width)===Math.round(t.width)}var oo;function _h(e,t){return function(){if(!oo){var i=arguments,r=this;i.length===1?e.call(r,i[0]):e.apply(r,i),oo=setTimeout(function(){oo=void 0},t)}}}function d_(){clearTimeout(oo),oo=void 0}function yh(e,t,i){e.scrollLeft+=t,e.scrollTop+=i}function xh(e){var t=window.Polymer,i=window.jQuery||window.Zepto;return t&&t.dom?t.dom(e).cloneNode(!0):i?i(e).clone(!0)[0]:e.cloneNode(!0)}function wh(e,t,i){var r={};return Array.from(e.children).forEach(function(o){var n,s,a,l;if(!(!Ye(o,t.draggable,e,!1)||o.animated||o===i)){var c=te(o);r.left=Math.min((n=r.left)!==null&&n!==void 0?n:1/0,c.left),r.top=Math.min((s=r.top)!==null&&s!==void 0?s:1/0,c.top),r.right=Math.max((a=r.right)!==null&&a!==void 0?a:-1/0,c.right),r.bottom=Math.max((l=r.bottom)!==null&&l!==void 0?l:-1/0,c.bottom)}}),r.width=r.right-r.left,r.height=r.bottom-r.top,r.x=r.left,r.y=r.top,r}var be="Sortable"+new Date().getTime();function h_(){var e=[],t;return{captureAnimationState:function(){if(e=[],!!this.options.animation){var r=[].slice.call(this.el.children);r.forEach(function(o){if(!(C(o,"display")==="none"||o===E.ghost)){e.push({target:o,rect:te(o)});var n=ut({},e[e.length-1].rect);if(o.thisAnimationDuration){var s=tr(o,!0);s&&(n.top-=s.f,n.left-=s.e)}o.fromRect=n}})}},addAnimationState:function(r){e.push(r)},removeAnimationState:function(r){e.splice(l_(e,{target:r}),1)},animateAll:function(r){var o=this;if(!this.options.animation){clearTimeout(t),typeof r=="function"&&r();return}var n=!1,s=0;e.forEach(function(a){var l=0,c=a.target,d=c.fromRect,h=te(c),f=c.prevFromRect,v=c.prevToRect,g=a.rect,_=tr(c,!0);_&&(h.top-=_.f,h.left-=_.e),c.toRect=h,c.thisAnimationDuration&&Is(f,h)&&!Is(d,h)&&(g.top-h.top)/(g.left-h.left)===(d.top-h.top)/(d.left-h.left)&&(l=p_(g,f,v,o.options)),Is(h,d)||(c.prevFromRect=d,c.prevToRect=h,l||(l=o.options.animation),o.animate(c,g,h,l)),l&&(n=!0,s=Math.max(s,l),clearTimeout(c.animationResetTimer),c.animationResetTimer=setTimeout(function(){c.animationTime=0,c.prevFromRect=null,c.fromRect=null,c.prevToRect=null,c.thisAnimationDuration=null},l),c.thisAnimationDuration=l)}),clearTimeout(t),n?t=setTimeout(function(){typeof r=="function"&&r()},s):typeof r=="function"&&r(),e=[]},animate:function(r,o,n,s){if(s){C(r,"transition",""),C(r,"transform","");var a=tr(this.el),l=a&&a.a,c=a&&a.d,d=(o.left-n.left)/(l||1),h=(o.top-n.top)/(c||1);r.animatingX=!!d,r.animatingY=!!h,C(r,"transform","translate3d("+d+"px,"+h+"px,0)"),this.forRepaintDummy=u_(r),C(r,"transition","transform "+s+"ms"+(this.options.easing?" "+this.options.easing:"")),C(r,"transform","translate3d(0,0,0)"),typeof r.animated=="number"&&clearTimeout(r.animated),r.animated=setTimeout(function(){C(r,"transition",""),C(r,"transform",""),r.animated=!1,r.animatingX=!1,r.animatingY=!1},s)}}}}function u_(e){return e.offsetWidth}function p_(e,t,i,r){return Math.sqrt(Math.pow(t.top-e.top,2)+Math.pow(t.left-e.left,2))/Math.sqrt(Math.pow(t.top-i.top,2)+Math.pow(t.left-i.left,2))*r.animation}var Vi=[],Ms={initializeByDefault:!0},Vo={mount:function(t){for(var i in Ms)Ms.hasOwnProperty(i)&&!(i in t)&&(t[i]=Ms[i]);Vi.forEach(function(r){if(r.pluginName===t.pluginName)throw"Sortable: Cannot mount plugin ".concat(t.pluginName," more than once")}),Vi.push(t)},pluginEvent:function(t,i,r){var o=this;this.eventCanceled=!1,r.cancel=function(){o.eventCanceled=!0};var n=t+"Global";Vi.forEach(function(s){i[s.pluginName]&&(i[s.pluginName][n]&&i[s.pluginName][n](ut({sortable:i},r)),i.options[s.pluginName]&&i[s.pluginName][t]&&i[s.pluginName][t](ut({sortable:i},r)))})},initializePlugins:function(t,i,r,o){Vi.forEach(function(a){var l=a.pluginName;if(!(!t.options[l]&&!a.initializeByDefault)){var c=new a(t,i,t.options);c.sortable=t,c.options=t.options,t[l]=c,Ot(r,c.defaults)}});for(var n in t.options)if(t.options.hasOwnProperty(n)){var s=this.modifyOption(t,n,t.options[n]);typeof s<"u"&&(t.options[n]=s)}},getEventProperties:function(t,i){var r={};return Vi.forEach(function(o){typeof o.eventProperties=="function"&&Ot(r,o.eventProperties.call(i[o.pluginName],t))}),r},modifyOption:function(t,i,r){var o;return Vi.forEach(function(n){t[n.pluginName]&&n.optionListeners&&typeof n.optionListeners[i]=="function"&&(o=n.optionListeners[i].call(t[n.pluginName],r))}),o}};function f_(e){var t=e.sortable,i=e.rootEl,r=e.name,o=e.targetEl,n=e.cloneEl,s=e.toEl,a=e.fromEl,l=e.oldIndex,c=e.newIndex,d=e.oldDraggableIndex,h=e.newDraggableIndex,f=e.originalEvent,v=e.putSortable,g=e.extraEventProperties;if(t=t||i&&i[be],!!t){var _,k=t.options,S="on"+r.charAt(0).toUpperCase()+r.substr(1);window.CustomEvent&&!Mt&&!jo?_=new CustomEvent(r,{bubbles:!0,cancelable:!0}):(_=document.createEvent("Event"),_.initEvent(r,!0,!0)),_.to=s||i,_.from=a||i,_.item=o||i,_.clone=n,_.oldIndex=l,_.newIndex=c,_.oldDraggableIndex=d,_.newDraggableIndex=h,_.originalEvent=f,_.pullMode=v?v.lastPutMode:void 0;var M=ut(ut({},g),Vo.getEventProperties(r,t));for(var N in M)_[N]=M[N];i&&i.dispatchEvent(_),k[S]&&k[S].call(t,_)}}var g_=["evt"],me=function(t,i){var r=arguments.length>2&&arguments[2]!==void 0?arguments[2]:{},o=r.evt,n=r_(r,g_);Vo.pluginEvent.bind(E)(t,i,ut({dragEl:x,parentEl:K,ghostEl:P,rootEl:j,nextEl:di,lastDownEl:$n,cloneEl:X,cloneHidden:Ht,dragStarted:Zr,putSortable:re,activeSortable:E.active,originalEvent:o,oldIndex:Zi,oldDraggableIndex:no,newIndex:Se,newDraggableIndex:Bt,hideGhostForTarget:Ch,unhideGhostForTarget:Eh,cloneNowHidden:function(){Ht=!0},cloneNowShown:function(){Ht=!1},dispatchSortableEvent:function(a){ce({sortable:i,name:a,originalEvent:o})}},n))};function ce(e){f_(ut({putSortable:re,cloneEl:X,targetEl:x,rootEl:j,oldIndex:Zi,oldDraggableIndex:no,newIndex:Se,newDraggableIndex:Bt},e))}var x,K,P,j,di,$n,X,Ht,Zi,Se,no,Bt,an,re,Ki=!1,Fn=!1,Hn=[],li,Ge,zs,Ns,$c,kc,Zr,qi,so,ao=!1,ln=!1,kn,oe,Rs=[],ca=!1,Un=[],fs=typeof document<"u",cn=ll,Sc=jo||Mt?"cssFloat":"float",m_=fs&&!gh&&!ll&&"draggable"in document.createElement("div"),$h=function(){if(fs){if(Mt)return!1;var e=document.createElement("x");return e.style.cssText="pointer-events:auto",e.style.pointerEvents==="auto"}}(),kh=function(t,i){var r=C(t),o=parseInt(r.width)-parseInt(r.paddingLeft)-parseInt(r.paddingRight)-parseInt(r.borderLeftWidth)-parseInt(r.borderRightWidth),n=yr(t,0,i),s=yr(t,1,i),a=n&&C(n),l=s&&C(s),c=a&&parseInt(a.marginLeft)+parseInt(a.marginRight)+te(n).width,d=l&&parseInt(l.marginLeft)+parseInt(l.marginRight)+te(s).width;if(r.display==="flex")return r.flexDirection==="column"||r.flexDirection==="column-reverse"?"vertical":"horizontal";if(r.display==="grid")return r.gridTemplateColumns.split(" ").length<=1?"vertical":"horizontal";if(n&&a.float&&a.float!=="none"){var h=a.float==="left"?"left":"right";return s&&(l.clear==="both"||l.clear===h)?"vertical":"horizontal"}return n&&(a.display==="block"||a.display==="flex"||a.display==="table"||a.display==="grid"||c>=o&&r[Sc]==="none"||s&&r[Sc]==="none"&&c+d>o)?"vertical":"horizontal"},v_=function(t,i,r){var o=r?t.left:t.top,n=r?t.right:t.bottom,s=r?t.width:t.height,a=r?i.left:i.top,l=r?i.right:i.bottom,c=r?i.width:i.height;return o===a||n===l||o+s/2===a+c/2},b_=function(t,i){var r;return Hn.some(function(o){var n=o[be].options.emptyInsertThreshold;if(!(!n||cl(o))){var s=te(o),a=t>=s.left-n&&t<=s.right+n,l=i>=s.top-n&&i<=s.bottom+n;if(a&&l)return r=o}}),r},Sh=function(t){function i(n,s){return function(a,l,c,d){var h=a.options.group.name&&l.options.group.name&&a.options.group.name===l.options.group.name;if(n==null&&(s||h))return!0;if(n==null||n===!1)return!1;if(s&&n==="clone")return n;if(typeof n=="function")return i(n(a,l,c,d),s)(a,l,c,d);var f=(s?a:l).options.group.name;return n===!0||typeof n=="string"&&n===f||n.join&&n.indexOf(f)>-1}}var r={},o=t.group;(!o||la(o)!="object")&&(o={name:o}),r.name=o.name,r.checkPull=i(o.pull,!0),r.checkPut=i(o.put),r.revertClone=o.revertClone,t.group=r},Ch=function(){!$h&&P&&C(P,"display","none")},Eh=function(){!$h&&P&&C(P,"display","")};fs&&!gh&&document.addEventListener("click",function(e){if(Fn)return e.preventDefault(),e.stopPropagation&&e.stopPropagation(),e.stopImmediatePropagation&&e.stopImmediatePropagation(),Fn=!1,!1},!0);var ci=function(t){if(x){t=t.touches?t.touches[0]:t;var i=b_(t.clientX,t.clientY);if(i){var r={};for(var o in t)t.hasOwnProperty(o)&&(r[o]=t[o]);r.target=r.rootEl=i,r.preventDefault=void 0,r.stopPropagation=void 0,i[be]._onDragOver(r)}}},__=function(t){x&&x.parentNode[be]._isOutsideThisEl(t.target)};function E(e,t){if(!(e&&e.nodeType&&e.nodeType===1))throw"Sortable: `el` must be an HTMLElement, not ".concat({}.toString.call(e));this.el=e,this.options=t=Ot({},t),e[be]=this;var i={group:null,sort:!0,disabled:!1,store:null,handle:null,draggable:/^[uo]l$/i.test(e.nodeName)?">li":">*",swapThreshold:1,invertSwap:!1,invertedSwapThreshold:null,removeCloneOnHide:!0,direction:function(){return kh(e,this.options)},ghostClass:"sortable-ghost",chosenClass:"sortable-chosen",dragClass:"sortable-drag",ignore:"a, img",filter:null,preventOnFilter:!0,animation:0,easing:null,setData:function(s,a){s.setData("Text",a.textContent)},dropBubble:!1,dragoverBubble:!1,dataIdAttr:"data-id",delay:0,delayOnTouchOnly:!1,touchStartThreshold:(Number.parseInt?Number:window).parseInt(window.devicePixelRatio,10)||1,forceFallback:!1,fallbackClass:"sortable-fallback",fallbackOnBody:!1,fallbackTolerance:0,fallbackOffset:{x:0,y:0},supportPointer:E.supportPointer!==!1&&"PointerEvent"in window&&(!ro||ll),emptyInsertThreshold:5};Vo.initializePlugins(this,e,i);for(var r in i)!(r in t)&&(t[r]=i[r]);Sh(t);for(var o in this)o.charAt(0)==="_"&&typeof this[o]=="function"&&(this[o]=this[o].bind(this));this.nativeDraggable=t.forceFallback?!1:m_,this.nativeDraggable&&(this.options.touchStartThreshold=1),t.supportPointer?I(e,"pointerdown",this._onTapStart):(I(e,"mousedown",this._onTapStart),I(e,"touchstart",this._onTapStart)),this.nativeDraggable&&(I(e,"dragover",this),I(e,"dragenter",this)),Hn.push(this.el),t.store&&t.store.get&&this.sort(t.store.get(this)||[]),Ot(this,h_())}E.prototype={constructor:E,_isOutsideThisEl:function(t){!this.el.contains(t)&&t!==this.el&&(qi=null)},_getDirection:function(t,i){return typeof this.options.direction=="function"?this.options.direction.call(this,t,i,x):this.options.direction},_onTapStart:function(t){if(t.cancelable){var i=this,r=this.el,o=this.options,n=o.preventOnFilter,s=t.type,a=t.touches&&t.touches[0]||t.pointerType&&t.pointerType==="touch"&&t,l=(a||t).target,c=t.target.shadowRoot&&(t.path&&t.path[0]||t.composedPath&&t.composedPath()[0])||l,d=o.filter;if(E_(r),!x&&!(/mousedown|pointerdown/.test(s)&&t.button!==0||o.disabled)&&!c.isContentEditable&&!(!this.nativeDraggable&&ro&&l&&l.tagName.toUpperCase()==="SELECT")&&(l=Ye(l,o.draggable,r,!1),!(l&&l.animated)&&$n!==l)){if(Zi=ze(l),no=ze(l,o.draggable),typeof d=="function"){if(d.call(this,t,l,this)){ce({sortable:i,rootEl:c,name:"filter",targetEl:l,toEl:r,fromEl:r}),me("filter",i,{evt:t}),n&&t.preventDefault();return}}else if(d&&(d=d.split(",").some(function(h){if(h=Ye(c,h.trim(),r,!1),h)return ce({sortable:i,rootEl:h,name:"filter",targetEl:l,fromEl:r,toEl:r}),me("filter",i,{evt:t}),!0}),d)){n&&t.preventDefault();return}o.handle&&!Ye(c,o.handle,r,!1)||this._prepareDragStart(t,a,l)}}},_prepareDragStart:function(t,i,r){var o=this,n=o.el,s=o.options,a=n.ownerDocument,l;if(r&&!x&&r.parentNode===n){var c=te(r);if(j=n,x=r,K=x.parentNode,di=x.nextSibling,$n=r,an=s.group,E.dragged=x,li={target:x,clientX:(i||t).clientX,clientY:(i||t).clientY},$c=li.clientX-c.left,kc=li.clientY-c.top,this._lastX=(i||t).clientX,this._lastY=(i||t).clientY,x.style["will-change"]="all",l=function(){if(me("delayEnded",o,{evt:t}),E.eventCanceled){o._onDrop();return}o._disableDelayedDragEvents(),!_c&&o.nativeDraggable&&(x.draggable=!0),o._triggerDragStart(t,i),ce({sortable:o,name:"choose",originalEvent:t}),ke(x,s.chosenClass,!0)},s.ignore.split(",").forEach(function(d){bh(x,d.trim(),Ls)}),I(a,"dragover",ci),I(a,"mousemove",ci),I(a,"touchmove",ci),s.supportPointer?(I(a,"pointerup",o._onDrop),!this.nativeDraggable&&I(a,"pointercancel",o._onDrop)):(I(a,"mouseup",o._onDrop),I(a,"touchend",o._onDrop),I(a,"touchcancel",o._onDrop)),_c&&this.nativeDraggable&&(this.options.touchStartThreshold=4,x.draggable=!0),me("delayStart",this,{evt:t}),s.delay&&(!s.delayOnTouchOnly||i)&&(!this.nativeDraggable||!(jo||Mt))){if(E.eventCanceled){this._onDrop();return}s.supportPointer?(I(a,"pointerup",o._disableDelayedDrag),I(a,"pointercancel",o._disableDelayedDrag)):(I(a,"mouseup",o._disableDelayedDrag),I(a,"touchend",o._disableDelayedDrag),I(a,"touchcancel",o._disableDelayedDrag)),I(a,"mousemove",o._delayedDragTouchMoveHandler),I(a,"touchmove",o._delayedDragTouchMoveHandler),s.supportPointer&&I(a,"pointermove",o._delayedDragTouchMoveHandler),o._dragStartTimer=setTimeout(l,s.delay)}else l()}},_delayedDragTouchMoveHandler:function(t){var i=t.touches?t.touches[0]:t;Math.max(Math.abs(i.clientX-this._lastX),Math.abs(i.clientY-this._lastY))>=Math.floor(this.options.touchStartThreshold/(this.nativeDraggable&&window.devicePixelRatio||1))&&this._disableDelayedDrag()},_disableDelayedDrag:function(){x&&Ls(x),clearTimeout(this._dragStartTimer),this._disableDelayedDragEvents()},_disableDelayedDragEvents:function(){var t=this.el.ownerDocument;D(t,"mouseup",this._disableDelayedDrag),D(t,"touchend",this._disableDelayedDrag),D(t,"touchcancel",this._disableDelayedDrag),D(t,"pointerup",this._disableDelayedDrag),D(t,"pointercancel",this._disableDelayedDrag),D(t,"mousemove",this._delayedDragTouchMoveHandler),D(t,"touchmove",this._delayedDragTouchMoveHandler),D(t,"pointermove",this._delayedDragTouchMoveHandler)},_triggerDragStart:function(t,i){i=i||t.pointerType=="touch"&&t,!this.nativeDraggable||i?this.options.supportPointer?I(document,"pointermove",this._onTouchMove):i?I(document,"touchmove",this._onTouchMove):I(document,"mousemove",this._onTouchMove):(I(x,"dragend",this),I(j,"dragstart",this._onDragStart));try{document.selection?Sn(function(){document.selection.empty()}):window.getSelection().removeAllRanges()}catch{}},_dragStarted:function(t,i){if(Ki=!1,j&&x){me("dragStarted",this,{evt:i}),this.nativeDraggable&&I(document,"dragover",__);var r=this.options;!t&&ke(x,r.dragClass,!1),ke(x,r.ghostClass,!0),E.active=this,t&&this._appendGhost(),ce({sortable:this,name:"start",originalEvent:i})}else this._nulling()},_emulateDragOver:function(){if(Ge){this._lastX=Ge.clientX,this._lastY=Ge.clientY,Ch();for(var t=document.elementFromPoint(Ge.clientX,Ge.clientY),i=t;t&&t.shadowRoot&&(t=t.shadowRoot.elementFromPoint(Ge.clientX,Ge.clientY),t!==i);)i=t;if(x.parentNode[be]._isOutsideThisEl(t),i)do{if(i[be]){var r=void 0;if(r=i[be]._onDragOver({clientX:Ge.clientX,clientY:Ge.clientY,target:t,rootEl:i}),r&&!this.options.dragoverBubble)break}t=i}while(i=vh(i));Eh()}},_onTouchMove:function(t){if(li){var i=this.options,r=i.fallbackTolerance,o=i.fallbackOffset,n=t.touches?t.touches[0]:t,s=P&&tr(P,!0),a=P&&s&&s.a,l=P&&s&&s.d,c=cn&&oe&&wc(oe),d=(n.clientX-li.clientX+o.x)/(a||1)+(c?c[0]-Rs[0]:0)/(a||1),h=(n.clientY-li.clientY+o.y)/(l||1)+(c?c[1]-Rs[1]:0)/(l||1);if(!E.active&&!Ki){if(r&&Math.max(Math.abs(n.clientX-this._lastX),Math.abs(n.clientY-this._lastY))<r)return;this._onDragStart(t,!0)}if(P){s?(s.e+=d-(zs||0),s.f+=h-(Ns||0)):s={a:1,b:0,c:0,d:1,e:d,f:h};var f="matrix(".concat(s.a,",").concat(s.b,",").concat(s.c,",").concat(s.d,",").concat(s.e,",").concat(s.f,")");C(P,"webkitTransform",f),C(P,"mozTransform",f),C(P,"msTransform",f),C(P,"transform",f),zs=d,Ns=h,Ge=n}t.cancelable&&t.preventDefault()}},_appendGhost:function(){if(!P){var t=this.options.fallbackOnBody?document.body:j,i=te(x,!0,cn,!0,t),r=this.options;if(cn){for(oe=t;C(oe,"position")==="static"&&C(oe,"transform")==="none"&&oe!==document;)oe=oe.parentNode;oe!==document.body&&oe!==document.documentElement?(oe===document&&(oe=at()),i.top+=oe.scrollTop,i.left+=oe.scrollLeft):oe=at(),Rs=wc(oe)}P=x.cloneNode(!0),ke(P,r.ghostClass,!1),ke(P,r.fallbackClass,!0),ke(P,r.dragClass,!0),C(P,"transition",""),C(P,"transform",""),C(P,"box-sizing","border-box"),C(P,"margin",0),C(P,"top",i.top),C(P,"left",i.left),C(P,"width",i.width),C(P,"height",i.height),C(P,"opacity","0.8"),C(P,"position",cn?"absolute":"fixed"),C(P,"zIndex","100000"),C(P,"pointerEvents","none"),E.ghost=P,t.appendChild(P),C(P,"transform-origin",$c/parseInt(P.style.width)*100+"% "+kc/parseInt(P.style.height)*100+"%")}},_onDragStart:function(t,i){var r=this,o=t.dataTransfer,n=r.options;if(me("dragStart",this,{evt:t}),E.eventCanceled){this._onDrop();return}me("setupClone",this),E.eventCanceled||(X=xh(x),X.removeAttribute("id"),X.draggable=!1,X.style["will-change"]="",this._hideClone(),ke(X,this.options.chosenClass,!1),E.clone=X),r.cloneId=Sn(function(){me("clone",r),!E.eventCanceled&&(r.options.removeCloneOnHide||j.insertBefore(X,x),r._hideClone(),ce({sortable:r,name:"clone"}))}),!i&&ke(x,n.dragClass,!0),i?(Fn=!0,r._loopId=setInterval(r._emulateDragOver,50)):(D(document,"mouseup",r._onDrop),D(document,"touchend",r._onDrop),D(document,"touchcancel",r._onDrop),o&&(o.effectAllowed="move",n.setData&&n.setData.call(r,o,x)),I(document,"drop",r),C(x,"transform","translateZ(0)")),Ki=!0,r._dragStartId=Sn(r._dragStarted.bind(r,i,t)),I(document,"selectstart",r),Zr=!0,window.getSelection().removeAllRanges(),ro&&C(document.body,"user-select","none")},_onDragOver:function(t){var i=this.el,r=t.target,o,n,s,a=this.options,l=a.group,c=E.active,d=an===l,h=a.sort,f=re||c,v,g=this,_=!1;if(ca)return;function k(Hr,cu){me(Hr,g,ut({evt:t,isOwner:d,axis:v?"vertical":"horizontal",revert:s,dragRect:o,targetRect:n,canSort:h,fromSortable:f,target:r,completed:M,onMove:function(Pl,du){return dn(j,i,x,o,Pl,te(Pl),t,du)},changed:N},cu))}function S(){k("dragOverAnimationCapture"),g.captureAnimationState(),g!==f&&f.captureAnimationState()}function M(Hr){return k("dragOverCompleted",{insertion:Hr}),Hr&&(d?c._hideClone():c._showClone(g),g!==f&&(ke(x,re?re.options.ghostClass:c.options.ghostClass,!1),ke(x,a.ghostClass,!0)),re!==g&&g!==E.active?re=g:g===E.active&&re&&(re=null),f===g&&(g._ignoreWhileAnimating=r),g.animateAll(function(){k("dragOverAnimationComplete"),g._ignoreWhileAnimating=null}),g!==f&&(f.animateAll(),f._ignoreWhileAnimating=null)),(r===x&&!x.animated||r===i&&!r.animated)&&(qi=null),!a.dragoverBubble&&!t.rootEl&&r!==document&&(x.parentNode[be]._isOutsideThisEl(t.target),!Hr&&ci(t)),!a.dragoverBubble&&t.stopPropagation&&t.stopPropagation(),_=!0}function N(){Se=ze(x),Bt=ze(x,a.draggable),ce({sortable:g,name:"change",toEl:i,newIndex:Se,newDraggableIndex:Bt,originalEvent:t})}if(t.preventDefault!==void 0&&t.cancelable&&t.preventDefault(),r=Ye(r,a.draggable,i,!0),k("dragOver"),E.eventCanceled)return _;if(x.contains(t.target)||r.animated&&r.animatingX&&r.animatingY||g._ignoreWhileAnimating===r)return M(!1);if(Fn=!1,c&&!a.disabled&&(d?h||(s=K!==j):re===this||(this.lastPutMode=an.checkPull(this,c,x,t))&&l.checkPut(this,c,x,t))){if(v=this._getDirection(t,r)==="vertical",o=te(x),k("dragOverValid"),E.eventCanceled)return _;if(s)return K=j,S(),this._hideClone(),k("revert"),E.eventCanceled||(di?j.insertBefore(x,di):j.appendChild(x)),M(!0);var R=cl(i,a.draggable);if(!R||$_(t,v,this)&&!R.animated){if(R===x)return M(!1);if(R&&i===t.target&&(r=R),r&&(n=te(r)),dn(j,i,x,o,r,n,t,!!r)!==!1)return S(),R&&R.nextSibling?i.insertBefore(x,R.nextSibling):i.appendChild(x),K=i,N(),M(!0)}else if(R&&w_(t,v,this)){var Q=yr(i,0,a,!0);if(Q===x)return M(!1);if(r=Q,n=te(r),dn(j,i,x,o,r,n,t,!1)!==!1)return S(),i.insertBefore(x,Q),K=i,N(),M(!0)}else if(r.parentNode===i){n=te(r);var B=0,J,ae=x.parentNode!==i,q=!v_(x.animated&&x.toRect||o,r.animated&&r.toRect||n,v),De=v?"top":"left",ge=xc(r,"top","top")||xc(x,"top","top"),Br=ge?ge.scrollTop:void 0;qi!==r&&(J=n[De],ao=!1,ln=!q&&a.invertSwap||ae),B=k_(t,r,n,v,q?1:a.swapThreshold,a.invertedSwapThreshold==null?a.swapThreshold:a.invertedSwapThreshold,ln,qi===r);var xt;if(B!==0){var ni=ze(x);do ni-=B,xt=K.children[ni];while(xt&&(C(xt,"display")==="none"||xt===P))}if(B===0||xt===r)return M(!1);qi=r,so=B;var Fr=r.nextElementSibling,zt=!1;zt=B===1;var Qo=dn(j,i,x,o,r,n,t,zt);if(Qo!==!1)return(Qo===1||Qo===-1)&&(zt=Qo===1),ca=!0,setTimeout(x_,30),S(),zt&&!Fr?i.appendChild(x):r.parentNode.insertBefore(x,zt?Fr:r),ge&&yh(ge,0,Br-ge.scrollTop),K=x.parentNode,J!==void 0&&!ln&&(kn=Math.abs(J-te(r)[De])),N(),M(!0)}if(i.contains(x))return M(!1)}return!1},_ignoreWhileAnimating:null,_offMoveEvents:function(){D(document,"mousemove",this._onTouchMove),D(document,"touchmove",this._onTouchMove),D(document,"pointermove",this._onTouchMove),D(document,"dragover",ci),D(document,"mousemove",ci),D(document,"touchmove",ci)},_offUpEvents:function(){var t=this.el.ownerDocument;D(t,"mouseup",this._onDrop),D(t,"touchend",this._onDrop),D(t,"pointerup",this._onDrop),D(t,"pointercancel",this._onDrop),D(t,"touchcancel",this._onDrop),D(document,"selectstart",this)},_onDrop:function(t){var i=this.el,r=this.options;if(Se=ze(x),Bt=ze(x,r.draggable),me("drop",this,{evt:t}),K=x&&x.parentNode,Se=ze(x),Bt=ze(x,r.draggable),E.eventCanceled){this._nulling();return}Ki=!1,ln=!1,ao=!1,clearInterval(this._loopId),clearTimeout(this._dragStartTimer),da(this.cloneId),da(this._dragStartId),this.nativeDraggable&&(D(document,"drop",this),D(i,"dragstart",this._onDragStart)),this._offMoveEvents(),this._offUpEvents(),ro&&C(document.body,"user-select",""),C(x,"transform",""),t&&(Zr&&(t.cancelable&&t.preventDefault(),!r.dropBubble&&t.stopPropagation()),P&&P.parentNode&&P.parentNode.removeChild(P),(j===K||re&&re.lastPutMode!=="clone")&&X&&X.parentNode&&X.parentNode.removeChild(X),x&&(this.nativeDraggable&&D(x,"dragend",this),Ls(x),x.style["will-change"]="",Zr&&!Ki&&ke(x,re?re.options.ghostClass:this.options.ghostClass,!1),ke(x,this.options.chosenClass,!1),ce({sortable:this,name:"unchoose",toEl:K,newIndex:null,newDraggableIndex:null,originalEvent:t}),j!==K?(Se>=0&&(ce({rootEl:K,name:"add",toEl:K,fromEl:j,originalEvent:t}),ce({sortable:this,name:"remove",toEl:K,originalEvent:t}),ce({rootEl:K,name:"sort",toEl:K,fromEl:j,originalEvent:t}),ce({sortable:this,name:"sort",toEl:K,originalEvent:t})),re&&re.save()):Se!==Zi&&Se>=0&&(ce({sortable:this,name:"update",toEl:K,originalEvent:t}),ce({sortable:this,name:"sort",toEl:K,originalEvent:t})),E.active&&((Se==null||Se===-1)&&(Se=Zi,Bt=no),ce({sortable:this,name:"end",toEl:K,originalEvent:t}),this.save()))),this._nulling()},_nulling:function(){me("nulling",this),j=x=K=P=di=X=$n=Ht=li=Ge=Zr=Se=Bt=Zi=no=qi=so=re=an=E.dragged=E.ghost=E.clone=E.active=null;var t=this.el;Un.forEach(function(i){t.contains(i)&&(i.checked=!0)}),Un.length=zs=Ns=0},handleEvent:function(t){switch(t.type){case"drop":case"dragend":this._onDrop(t);break;case"dragenter":case"dragover":x&&(this._onDragOver(t),y_(t));break;case"selectstart":t.preventDefault();break}},toArray:function(){for(var t=[],i,r=this.el.children,o=0,n=r.length,s=this.options;o<n;o++)i=r[o],Ye(i,s.draggable,this.el,!1)&&t.push(i.getAttribute(s.dataIdAttr)||C_(i));return t},sort:function(t,i){var r={},o=this.el;this.toArray().forEach(function(n,s){var a=o.children[s];Ye(a,this.options.draggable,o,!1)&&(r[n]=a)},this),i&&this.captureAnimationState(),t.forEach(function(n){r[n]&&(o.removeChild(r[n]),o.appendChild(r[n]))}),i&&this.animateAll()},save:function(){var t=this.options.store;t&&t.set&&t.set(this)},closest:function(t,i){return Ye(t,i||this.options.draggable,this.el,!1)},option:function(t,i){var r=this.options;if(i===void 0)return r[t];var o=Vo.modifyOption(this,t,i);typeof o<"u"?r[t]=o:r[t]=i,t==="group"&&Sh(r)},destroy:function(){me("destroy",this);var t=this.el;t[be]=null,D(t,"mousedown",this._onTapStart),D(t,"touchstart",this._onTapStart),D(t,"pointerdown",this._onTapStart),this.nativeDraggable&&(D(t,"dragover",this),D(t,"dragenter",this)),Array.prototype.forEach.call(t.querySelectorAll("[draggable]"),function(i){i.removeAttribute("draggable")}),this._onDrop(),this._disableDelayedDragEvents(),Hn.splice(Hn.indexOf(this.el),1),this.el=t=null},_hideClone:function(){if(!Ht){if(me("hideClone",this),E.eventCanceled)return;C(X,"display","none"),this.options.removeCloneOnHide&&X.parentNode&&X.parentNode.removeChild(X),Ht=!0}},_showClone:function(t){if(t.lastPutMode!=="clone"){this._hideClone();return}if(Ht){if(me("showClone",this),E.eventCanceled)return;x.parentNode==j&&!this.options.group.revertClone?j.insertBefore(X,x):di?j.insertBefore(X,di):j.appendChild(X),this.options.group.revertClone&&this.animate(x,X),C(X,"display",""),Ht=!1}}};function y_(e){e.dataTransfer&&(e.dataTransfer.dropEffect="move"),e.cancelable&&e.preventDefault()}function dn(e,t,i,r,o,n,s,a){var l,c=e[be],d=c.options.onMove,h;return window.CustomEvent&&!Mt&&!jo?l=new CustomEvent("move",{bubbles:!0,cancelable:!0}):(l=document.createEvent("Event"),l.initEvent("move",!0,!0)),l.to=t,l.from=e,l.dragged=i,l.draggedRect=r,l.related=o||t,l.relatedRect=n||te(t),l.willInsertAfter=a,l.originalEvent=s,e.dispatchEvent(l),d&&(h=d.call(c,l,s)),h}function Ls(e){e.draggable=!1}function x_(){ca=!1}function w_(e,t,i){var r=te(yr(i.el,0,i.options,!0)),o=wh(i.el,i.options,P),n=10;return t?e.clientX<o.left-n||e.clientY<r.top&&e.clientX<r.right:e.clientY<o.top-n||e.clientY<r.bottom&&e.clientX<r.left}function $_(e,t,i){var r=te(cl(i.el,i.options.draggable)),o=wh(i.el,i.options,P),n=10;return t?e.clientX>o.right+n||e.clientY>r.bottom&&e.clientX>r.left:e.clientY>o.bottom+n||e.clientX>r.right&&e.clientY>r.top}function k_(e,t,i,r,o,n,s,a){var l=r?e.clientY:e.clientX,c=r?i.height:i.width,d=r?i.top:i.left,h=r?i.bottom:i.right,f=!1;if(!s){if(a&&kn<c*o){if(!ao&&(so===1?l>d+c*n/2:l<h-c*n/2)&&(ao=!0),ao)f=!0;else if(so===1?l<d+kn:l>h-kn)return-so}else if(l>d+c*(1-o)/2&&l<h-c*(1-o)/2)return S_(t)}return f=f||s,f&&(l<d+c*n/2||l>h-c*n/2)?l>d+c/2?1:-1:0}function S_(e){return ze(x)<ze(e)?1:-1}function C_(e){for(var t=e.tagName+e.className+e.src+e.href+e.textContent,i=t.length,r=0;i--;)r+=t.charCodeAt(i);return r.toString(36)}function E_(e){Un.length=0;for(var t=e.getElementsByTagName("input"),i=t.length;i--;){var r=t[i];r.checked&&Un.push(r)}}function Sn(e){return setTimeout(e,0)}function da(e){return clearTimeout(e)}fs&&I(document,"touchmove",function(e){(E.active||Ki)&&e.cancelable&&e.preventDefault()});E.utils={on:I,off:D,css:C,find:bh,is:function(t,i){return!!Ye(t,i,t,!1)},extend:c_,throttle:_h,closest:Ye,toggleClass:ke,clone:xh,index:ze,nextTick:Sn,cancelNextTick:da,detectDirection:kh,getChild:yr,expando:be};E.get=function(e){return e[be]};E.mount=function(){for(var e=arguments.length,t=new Array(e),i=0;i<e;i++)t[i]=arguments[i];t[0].constructor===Array&&(t=t[0]),t.forEach(function(r){if(!r.prototype||!r.prototype.constructor)throw"Sortable: Mounted plugin must be a constructor function, not ".concat({}.toString.call(r));r.utils&&(E.utils=ut(ut({},E.utils),r.utils)),Vo.mount(r)})};E.create=function(e,t){return new E(e,t)};E.version=a_;var ee=[],Qr,ha,ua=!1,Bs,Fs,jn,Jr;function P_(){function e(){this.defaults={scroll:!0,forceAutoScrollFallback:!1,scrollSensitivity:30,scrollSpeed:10,bubbleScroll:!0};for(var t in this)t.charAt(0)==="_"&&typeof this[t]=="function"&&(this[t]=this[t].bind(this))}return e.prototype={dragStarted:function(i){var r=i.originalEvent;this.sortable.nativeDraggable?I(document,"dragover",this._handleAutoScroll):this.options.supportPointer?I(document,"pointermove",this._handleFallbackAutoScroll):r.touches?I(document,"touchmove",this._handleFallbackAutoScroll):I(document,"mousemove",this._handleFallbackAutoScroll)},dragOverCompleted:function(i){var r=i.originalEvent;!this.options.dragOverBubble&&!r.rootEl&&this._handleAutoScroll(r)},drop:function(){this.sortable.nativeDraggable?D(document,"dragover",this._handleAutoScroll):(D(document,"pointermove",this._handleFallbackAutoScroll),D(document,"touchmove",this._handleFallbackAutoScroll),D(document,"mousemove",this._handleFallbackAutoScroll)),Cc(),Cn(),d_()},nulling:function(){jn=ha=Qr=ua=Jr=Bs=Fs=null,ee.length=0},_handleFallbackAutoScroll:function(i){this._handleAutoScroll(i,!0)},_handleAutoScroll:function(i,r){var o=this,n=(i.touches?i.touches[0]:i).clientX,s=(i.touches?i.touches[0]:i).clientY,a=document.elementFromPoint(n,s);if(jn=i,r||this.options.forceAutoScrollFallback||jo||Mt||ro){Hs(i,this.options,a,r);var l=jt(a,!0);ua&&(!Jr||n!==Bs||s!==Fs)&&(Jr&&Cc(),Jr=setInterval(function(){var c=jt(document.elementFromPoint(n,s),!0);c!==l&&(l=c,Cn()),Hs(i,o.options,c,r)},10),Bs=n,Fs=s)}else{if(!this.options.bubbleScroll||jt(a,!0)===at()){Cn();return}Hs(i,this.options,jt(a,!1),!1)}}},Ot(e,{pluginName:"scroll",initializeByDefault:!0})}function Cn(){ee.forEach(function(e){clearInterval(e.pid)}),ee=[]}function Cc(){clearInterval(Jr)}var Hs=_h(function(e,t,i,r){if(t.scroll){var o=(e.touches?e.touches[0]:e).clientX,n=(e.touches?e.touches[0]:e).clientY,s=t.scrollSensitivity,a=t.scrollSpeed,l=at(),c=!1,d;ha!==i&&(ha=i,Cn(),Qr=t.scroll,d=t.scrollFn,Qr===!0&&(Qr=jt(i,!0)));var h=0,f=Qr;do{var v=f,g=te(v),_=g.top,k=g.bottom,S=g.left,M=g.right,N=g.width,R=g.height,Q=void 0,B=void 0,J=v.scrollWidth,ae=v.scrollHeight,q=C(v),De=v.scrollLeft,ge=v.scrollTop;v===l?(Q=N<J&&(q.overflowX==="auto"||q.overflowX==="scroll"||q.overflowX==="visible"),B=R<ae&&(q.overflowY==="auto"||q.overflowY==="scroll"||q.overflowY==="visible")):(Q=N<J&&(q.overflowX==="auto"||q.overflowX==="scroll"),B=R<ae&&(q.overflowY==="auto"||q.overflowY==="scroll"));var Br=Q&&(Math.abs(M-o)<=s&&De+N<J)-(Math.abs(S-o)<=s&&!!De),xt=B&&(Math.abs(k-n)<=s&&ge+R<ae)-(Math.abs(_-n)<=s&&!!ge);if(!ee[h])for(var ni=0;ni<=h;ni++)ee[ni]||(ee[ni]={});(ee[h].vx!=Br||ee[h].vy!=xt||ee[h].el!==v)&&(ee[h].el=v,ee[h].vx=Br,ee[h].vy=xt,clearInterval(ee[h].pid),(Br!=0||xt!=0)&&(c=!0,ee[h].pid=setInterval((function(){r&&this.layer===0&&E.active._onTouchMove(jn);var Fr=ee[this.layer].vy?ee[this.layer].vy*a:0,zt=ee[this.layer].vx?ee[this.layer].vx*a:0;typeof d=="function"&&d.call(E.dragged.parentNode[be],zt,Fr,e,jn,ee[this.layer].el)!=="continue"||yh(ee[this.layer].el,zt,Fr)}).bind({layer:h}),24))),h++}while(t.bubbleScroll&&f!==l&&(f=jt(f,!1)));ua=c}},30),Ph=function(t){var i=t.originalEvent,r=t.putSortable,o=t.dragEl,n=t.activeSortable,s=t.dispatchSortableEvent,a=t.hideGhostForTarget,l=t.unhideGhostForTarget;if(i){var c=r||n;a();var d=i.changedTouches&&i.changedTouches.length?i.changedTouches[0]:i,h=document.elementFromPoint(d.clientX,d.clientY);l(),c&&!c.el.contains(h)&&(s("spill"),this.onSpill({dragEl:o,putSortable:r}))}};function dl(){}dl.prototype={startIndex:null,dragStart:function(t){var i=t.oldDraggableIndex;this.startIndex=i},onSpill:function(t){var i=t.dragEl,r=t.putSortable;this.sortable.captureAnimationState(),r&&r.captureAnimationState();var o=yr(this.sortable.el,this.startIndex,this.options);o?this.sortable.el.insertBefore(i,o):this.sortable.el.appendChild(i),this.sortable.animateAll(),r&&r.animateAll()},drop:Ph};Ot(dl,{pluginName:"revertOnSpill"});function hl(){}hl.prototype={onSpill:function(t){var i=t.dragEl,r=t.putSortable,o=r||this.sortable;o.captureAnimationState(),i.parentNode&&i.parentNode.removeChild(i),o.animateAll()},drop:Ph};Ot(hl,{pluginName:"removeOnSpill"});E.mount(new P_);E.mount(hl,dl);var T_=Object.defineProperty,O_=Object.getOwnPropertyDescriptor,mt=(e,t,i,r)=>{for(var o=r>1?void 0:r?O_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&T_(t,i,o),o};let Ze=class extends y{constructor(){super(...arguments),this.disabled=!1,this.noStyle=!1,this.invertSwap=!1,this.rollback=!0,this._shouldDestroy=!1,this._handleUpdate=e=>{Z(this,"item-moved",{newIndex:e.newIndex,oldIndex:e.oldIndex})},this._handleAdd=e=>{Z(this,"item-added",{index:e.newIndex,data:e.item.sortableData,item:e.item})},this._handleRemove=e=>{Z(this,"item-removed",{index:e.oldIndex})},this._handleEnd=e=>{if(Z(this,"drag-end"),this.rollback&&e.item.placeholder){const t=e.item;t.placeholder.replaceWith(t),delete t.placeholder}},this._handleStart=()=>{Z(this,"drag-start")},this._handleChoose=e=>{if(!this.rollback)return;const t=e.item;t.placeholder=document.createComment("sort-placeholder"),t.after(t.placeholder)}}updated(e){e.has("disabled")&&this._ensureSortable()}disconnectedCallback(){super.disconnectedCallback(),this._shouldDestroy=!0,setTimeout(()=>{this._shouldDestroy&&(this._destroySortable(),this._shouldDestroy=!1)},1)}connectedCallback(){super.connectedCallback(),this._shouldDestroy=!1,this.hasUpdated&&!this._sortable&&!this.disabled&&this._ensureSortable()}createRenderRoot(){return this}render(){return this.noStyle?m:u`
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
    `}firstUpdated(){this.disabled||this._ensureSortable()}async _ensureSortable(){if(this._sortable){this._sortable.option("disabled",this.disabled);return}this.disabled||await this._createSortable()}async _createSortable(){if(this._sortable)return;const e=this.children[0];if(!e)return;const t={scroll:!0,forceAutoScrollFallback:!0,scrollSpeed:20,animation:150,disabled:this.disabled,...this.options,onChoose:this._handleChoose,onStart:this._handleStart,onEnd:this._handleEnd,onUpdate:this._handleUpdate,onAdd:this._handleAdd,onRemove:this._handleRemove};this.draggableSelector&&(t.draggable=this.draggableSelector),this.handleSelector&&(t.handle=this.handleSelector),this.invertSwap!==void 0&&(t.invertSwap=this.invertSwap),this.group&&(t.group=this.group),this.filter&&(t.filter=this.filter),this._sortable=E.create(e,t)}_destroySortable(){var e;(e=this._sortable)==null||e.destroy(),this._sortable=void 0}};mt([p({type:Boolean})],Ze.prototype,"disabled",2);mt([p({type:Boolean,attribute:"no-style"})],Ze.prototype,"noStyle",2);mt([p({type:String,attribute:"draggable-selector"})],Ze.prototype,"draggableSelector",2);mt([p({type:String,attribute:"handle-selector"})],Ze.prototype,"handleSelector",2);mt([p({type:String})],Ze.prototype,"filter",2);mt([p({attribute:!1})],Ze.prototype,"group",2);mt([p({type:Boolean,attribute:"invert-swap"})],Ze.prototype,"invertSwap",2);mt([p({attribute:!1})],Ze.prototype,"options",2);mt([p({type:Boolean})],Ze.prototype,"rollback",2);Ze=mt([w("ha-sortable")],Ze);function A_(e,t,i){if(!t.startsWith("/demo/"))return;const r=e.getState(t);if(r){if(t.includes("climate")||t.includes("thermostat")||t==="/demo/ac"){typeof i=="number"&&(typeof r.state=="number"?e.applyUpdate(t,i,r.attributes):e.applyUpdate(t,r.state,{...r.attributes,temperature:i}));return}if(t.includes("gain")&&typeof i=="number"){e.applyUpdate(t,i,r.attributes);return}if(t.includes("hsv")){const o=tl(i);o&&e.applyUpdate(t,eh(o),r.attributes);return}if(t.includes("rgba")||t.includes("color")){const o=el(i);o&&e.applyUpdate(t,Qd(o),r.attributes);return}if(t.includes("fader")||t.includes("vector")){if(Array.isArray(i)){const o=rh(i,i.length,{min:-1/0,max:1/0,step:0});e.applyUpdate(t,oh(o),r.attributes)}return}if(t.includes("xy")){const o=rl(i);o?e.applyUpdate(t,nh(o),r.attributes):typeof i=="number"&&e.applyUpdate(t,i,r.attributes);return}if(t.includes("shutter")||t.includes("cover")){if(typeof i=="number"){const o=Math.round(i),n=o<=0;e.applyUpdate(t,n?"closed":"open",{...r.attributes,current_position:o})}return}if(t.includes("trigger")||t.includes("button")){e.applyUpdate(t,!0,r.attributes),window.setTimeout(()=>{const o=e.getState(t);e.applyUpdate(t,!1,(o==null?void 0:o.attributes)??r.attributes)},700);return}e.applyUpdate(t,i,r.attributes)}}function D_(e,t){let i=0;return e.subscribeAll(()=>{i+=1}),{get connected(){return t.connected},get states(){return e.getStatesRecord()},get statesRevision(){return i},connection:t,subscribeStates(r){return t.subscribeStates(r)},subscribeEntity(r,o){return e.subscribe(r,o)},async callService(r,o){t.sendCommand(r,o),t.connected||A_(e,r,o)},formatState(r){const o=e.getState(r);if(!o)return"—";const n=o.state;if(typeof n=="boolean")return n?"开启":"关闭";if(Array.isArray(n)){if(r.includes("hsv")&&n.length===3){const l=tl(n);if(l)return th(l)}const a=el(n);return a?Jd(a):n.join(", ")}if(n==null)return"—";const s=o.attributes.unit_of_measurement;return s?`${n} ${s}`:String(n)}}}function I_(e){var n,s,a;if(!e)return[];const t=new Set;e.entity&&t.add(e.entity);const i=e.mute_entity;typeof i=="string"&&i&&t.add(i);const r=e.entity_x;typeof r=="string"&&r&&t.add(r);const o=e.entity_y;return typeof o=="string"&&o&&t.add(o),(n=e.badges)==null||n.forEach(l=>{l.entity&&t.add(l.entity)}),(s=e.elements)==null||s.forEach(l=>{l.entity&&t.add(l.entity)}),(a=e.entities)==null||a.forEach(l=>{l.entity&&t.add(l.entity)}),[...t]}function Ec(e,t){const i=document.createElement("hui-error-card");return i.setConfig({type:"error",message:e,origConfig:t}),i}async function ul(e){let t=e;e.type==="button"?t={...e,type:"trigger"}:e.type==="color"&&(t={...e,type:"rgba"});try{const i=await ya(t.type);if(!i)return Ec(`Unknown card type: ${t.type}`,t);const r=new i;return r.setConfig(t),r}catch(i){const r=i instanceof Error?i.message:String(i);return Ec(r,e)}}const Vt=3;function Us(e,t){return typeof e=="number"?e*t:e}function Th(e){const t={columns:Us(e.grid_columns,Vt),max_columns:Us(e.grid_max_columns,Vt),min_columns:Us(e.grid_min_columns,Vt),rows:e.grid_rows,max_rows:e.grid_max_rows,min_rows:e.grid_min_rows};for(const[i,r]of Object.entries(t))r===void 0&&delete t[i];return t}const ir={columns:12,rows:"auto"};function Pc(e,t,i){let r=e;return t!==void 0&&(r=Math.max(r,t)),i!==void 0&&(r=Math.min(r,i)),r}function Oh(e={}){const t=e.rows??"auto",i=e.columns??12,r=e.min_rows,o=e.max_rows,n=e.min_columns,s=e.max_columns,a=typeof t=="string"?t:Pc(t,r,o),l=typeof i=="string"?i:Pc(i,n,s);return{rows:a,columns:l}}function M_(e){const t=e.columns==="full",i=e.rows==="auto",r=t?12:e.columns,o=i?1:e.rows;return{columnSize:r,rowSize:o,fullWidth:t,autoHeight:i}}function Tc(e){return typeof e.columns=="number"&&e.columns%Vt!==0}const z_={columns:12,rows:"auto"};function N_(e){return e!=null&&e.grid_options?e.grid_options:e!=null&&e.layout_options?Th(e.layout_options):{}}function R_(e={},t){const i=N_(t),r={...z_,...e,...i};return typeof e.min_rows=="number"&&typeof r.rows=="number"&&r.rows<e.min_rows&&(r.rows=e.min_rows),typeof e.min_columns=="number"&&typeof r.columns=="number"&&r.columns<e.min_columns&&(r.columns=e.min_columns),r}function L_(e,t){var i;return R_(((i=e==null?void 0:e.getGridOptions)==null?void 0:i.call(e))??{},t)}const pa="flow-edit-menu-open";function B_(e){document.dispatchEvent(new CustomEvent(pa,{bubbles:!0,composed:!0,detail:{source:e}}))}function pl(e){const t=i=>{var o;const r=(o=i.detail)==null?void 0:o.source;r&&e(r)};return document.addEventListener(pa,t),()=>document.removeEventListener(pa,t)}function F_(e,t,i={}){const r=i.margin??8,o=i.gap??4,n=i.preferredMaxHeight??360,s=i.minMenuWidth??160,a=Math.max(s,t.width),l=window.innerHeight-e.bottom-r,c=e.top-r,d=Math.max(t.height,1),h=l>=d+o||l>=c,f=h?"bottom-end":"top-end",v=h?l-o:c-o,g=Math.max(120,Math.min(n,v)),_=Math.min(d,g);let k=h?e.bottom+o:e.top-_-o;k=Math.max(r,k);let S=e.right-a;return S=Math.max(r,Math.min(S,window.innerWidth-a-r)),{top:k,left:S,width:a,maxHeight:g,placement:f}}function fl(e,t=320){const i=e.getBoundingClientRect(),r=8,o=4,n=Math.max(i.width,160),s=180,a=window.innerHeight-i.bottom-r,l=i.top-r,c=a>=s+o||a>=l,d=Math.max(120,Math.min(t,(c?a:l)-o)),h=Math.min(s,d);return{top:c?i.bottom+o:Math.max(r,i.top-h-o),left:i.left,width:n,maxHeight:d}}var H_=Object.defineProperty,U_=Object.getOwnPropertyDescriptor,gs=(e,t,i,r)=>{for(var o=r>1?void 0:r?U_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&H_(t,i,o),o};const js=160;let xr=class extends y{constructor(){super(...arguments),this.items=[],this._open=!1,this._menuStyle={},this._onDocumentClick=e=>{this._open&&(e.composedPath().includes(this)||this._close())},this._repositionMenu=()=>{this._open&&this._updateMenuPosition()}}connectedCallback(){super.connectedCallback(),document.addEventListener("click",this._onDocumentClick),window.addEventListener("resize",this._repositionMenu),this._unsubMenuOpen=pl(e=>{e!==this&&this._open&&this._close()})}disconnectedCallback(){var e;document.removeEventListener("click",this._onDocumentClick),window.removeEventListener("resize",this._repositionMenu),this._detachScrollListener(),(e=this._unsubMenuOpen)==null||e.call(this),super.disconnectedCallback()}_attachScrollListener(){this._detachScrollListener();let e=this.parentElement;for(;e;){const t=getComputedStyle(e);if(/(auto|scroll)/.test(t.overflowY)){this._scrollParent=e,e.addEventListener("scroll",this._repositionMenu,{passive:!0});return}e=e.parentElement}}_detachScrollListener(){var e;(e=this._scrollParent)==null||e.removeEventListener("scroll",this._repositionMenu),this._scrollParent=void 0}_measureMenu(e){const t=Math.max(e.offsetWidth,js),i=Math.max(e.scrollHeight,e.offsetHeight);return{width:t,height:i}}_updateMenuPosition(){var n,s;const e=(n=this.shadowRoot)==null?void 0:n.querySelector(".trigger"),t=(s=this.shadowRoot)==null?void 0:s.querySelector(".menu");if(!e||!t)return!1;const i=e.getBoundingClientRect();if(i.width===0&&i.height===0)return!1;const r=this._measureMenu(t),o=F_(i,r,{preferredMaxHeight:360,minMenuWidth:js});return this._menuStyle={position:"fixed",top:`${o.top}px`,left:`${o.left}px`,width:`${o.width}px`,maxHeight:`${o.maxHeight}px`,zIndex:"200",visibility:"visible"},!0}async _scheduleMenuPosition(){if(await this.updateComplete,!!this._open){if(!this._updateMenuPosition()){if(await new Promise(e=>requestAnimationFrame(()=>e())),!this._open)return;this._updateMenuPosition()}await new Promise(e=>requestAnimationFrame(()=>e())),this._open&&this._updateMenuPosition()}}updated(e){super.updated(e),e.has("_open")&&this._open&&this._scheduleMenuPosition()}_openMenu(e){if(e.stopPropagation(),this._open){this._close();return}B_(this),this._menuStyle={position:"fixed",visibility:"hidden",width:`${js}px`},this._open=!0,this.setAttribute("open",""),this._attachScrollListener(),this.dispatchEvent(new CustomEvent("menu-opened",{bubbles:!0,composed:!0}))}_close(){this._open&&(this._open=!1,this._menuStyle={},this.removeAttribute("open"),this._detachScrollListener(),this.dispatchEvent(new CustomEvent("menu-closed",{bubbles:!0,composed:!0})))}_select(e,t){t.stopPropagation(),this._close(),this.dispatchEvent(new CustomEvent("action",{bubbles:!0,composed:!0,detail:{value:e}}))}render(){return u`
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
        ${this._open?u`
              <div class="menu" style=${T(this._menuStyle)} role="menu" @click=${e=>e.stopPropagation()}>
                ${this.items.map(e=>e.divider?u`<div class="divider"></div>`:u`
                        <button
                          type="button"
                          class="item ${e.destructive?"destructive":""}"
                          @click=${t=>this._select(e.value,t)}
                        >
                          ${e.icon?e.icon.startsWith("mdi:")?u`<ha-icon class="icon" .icon=${e.icon}></ha-icon>`:u`<span class="icon">${e.icon}</span>`:m}
                          <span>${e.label}</span>
                        </button>
                      `)}
              </div>
            `:m}
      </div>
    `}};xr.styles=$`
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
  `;gs([p({attribute:!1})],xr.prototype,"items",2);gs([b()],xr.prototype,"_open",2);gs([b()],xr.prototype,"_menuStyle",2);xr=gs([w("ha-action-menu")],xr);var j_=Object.defineProperty,V_=Object.getOwnPropertyDescriptor,vt=(e,t,i,r)=>{for(var o=r>1?void 0:r?V_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&j_(t,i,o),o};let Be=class extends y{constructor(){super(...arguments),this.sectionIndex=0,this.cardIndex=0,this.noEdit=!1,this.hiddenOverlay=!1,this.compact=!1,this.inactive=!1,this._hover=!1,this._focused=!1,this._menuOpen=!1,this._touchStarted=!1,this._documentClicked=e=>{this._hover=e.composedPath().includes(this),document.removeEventListener("click",this._documentClicked)}}connectedCallback(){super.connectedCallback(),this._unsubMenuOpen=pl(e=>{this.contains(e)||(this._hover=!1,this._focused=!1,this._menuOpen=!1)}),this.addEventListener("focus",()=>{this._focused=!0}),this.addEventListener("blur",()=>{this._focused=!1}),this.addEventListener("touchstart",()=>{this._touchStarted=!0,this._hover=!0,document.addEventListener("click",this._documentClicked)}),this.addEventListener("touchend",()=>{setTimeout(()=>{this._touchStarted=!1},10)}),this.addEventListener("mouseenter",()=>{this._touchStarted||(this._hover=!0)}),this.addEventListener("mouseleave",()=>{this._menuOpen||(this._hover=!1)}),this.addEventListener("click",()=>{this._hover=!0,document.addEventListener("click",this._documentClicked)})}disconnectedCallback(){var e;document.removeEventListener("click",this._documentClicked),(e=this._unsubMenuOpen)==null||e.call(this),super.disconnectedCallback()}updated(e){e.has("inactive")&&this.inactive&&(this._hover=!1,this._focused=!1,this._menuOpen=!1,this.toggleAttribute("menu-open",!1))}_path(){return{sectionIndex:this.sectionIndex,cardIndex:this.cardIndex}}_menuItems(){const e=[];return this.noEdit||e.push({value:"edit",label:"编辑",icon:"mdi:pencil"}),e.push({value:"duplicate",label:"创建副本",icon:"mdi:plus-circle-multiple-outline"}),e.push({value:"copy",label:"复制",icon:"mdi:content-copy"}),e.push({value:"cut",label:"剪切",icon:"mdi:content-cut"}),e.length&&e.push({value:"divider",label:"",divider:!0}),e.push({value:"delete",label:"删除",icon:"mdi:delete",destructive:!0}),e}_onMenuAction(e){const t=this._path();switch(e.detail.value){case"edit":Z(this,"ll-edit-card",t);break;case"duplicate":Z(this,"ll-duplicate-card",t);break;case"copy":Z(this,"ll-copy-card",t);break;case"cut":Z(this,"ll-copy-card",t),Z(this,"ll-delete-card",{...t,silent:!0});break;case"delete":Z(this,"ll-delete-card",{...t,silent:!1});break}}_onMenuOpened(){this._menuOpen=!0,this.toggleAttribute("menu-open",!0),this._hover=!0}_onMenuClosed(){this._menuOpen=!1,this.toggleAttribute("menu-open",!1)}_editOverlay(e){this.noEdit||(e.preventDefault(),e.stopPropagation(),Z(this,"ll-edit-card",this._path()))}render(){if(this.inactive)return u`
        <div class="card-wrapper">
          <slot></slot>
        </div>
      `;const e=this.hiddenOverlay||this.compact,t=(this._hover||this._focused||this._menuOpen)&&!e,i=this._hover||this._focused||this._menuOpen,r=this._hover||this._focused||this._menuOpen;return u`
      <div class="card-wrapper ${r?"elevated":""}">
        <slot></slot>
        <div
          class=${O({"card-overlay":!0,visible:t,compact:e,"menu-visible":i})}
        >
          ${e||this.noEdit?m:u`
                <button type="button" class="control" @click=${this._editOverlay} title="编辑">
                  <div class="control-overlay"></div>
                  <ha-icon class="control-icon" icon="mdi:pencil"></ha-icon>
                </button>
              `}
          ${this.noEdit&&!e?u`
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
    `}};Be.styles=$`
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
  `;vt([p({type:Number})],Be.prototype,"sectionIndex",2);vt([p({type:Number})],Be.prototype,"cardIndex",2);vt([p({type:Boolean,attribute:"no-edit"})],Be.prototype,"noEdit",2);vt([p({type:Boolean,attribute:"hidden-overlay"})],Be.prototype,"hiddenOverlay",2);vt([p({type:Boolean})],Be.prototype,"compact",2);vt([p({type:Boolean,reflect:!0})],Be.prototype,"inactive",2);vt([b()],Be.prototype,"_hover",2);vt([b()],Be.prototype,"_focused",2);vt([b()],Be.prototype,"_menuOpen",2);Be=vt([w("hui-card-edit-mode")],Be);var q_=Object.defineProperty,G_=Object.getOwnPropertyDescriptor,bt=(e,t,i,r)=>{for(var o=r>1?void 0:r?G_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&q_(t,i,o),o};let Fe=class extends y{constructor(){super(...arguments),this.sectionIndex=0,this.cardIndex=0,this.editMode=!1,this.preview=!1,this._statesRevision=0,this._configKey="",this._entityUnsubs=[],this._lastType=""}_refreshCard(){var t;const e=this._card;(t=e==null?void 0:e.requestUpdate)==null||t.call(e),this.requestUpdate()}connectedCallback(){super.connectedCallback()}disconnectedCallback(){this._clearEntitySubscriptions(),super.disconnectedCallback()}_clearEntitySubscriptions(){this._entityUnsubs.forEach(e=>e()),this._entityUnsubs=[]}_syncEntitySubscriptions(){this._clearEntitySubscriptions();const e=this.flow;if(!e||!this._card)return;const t=I_(this.config);if(t.length===0){this._entityUnsubs.push(e.subscribeStates(()=>{this._statesRevision=e.statesRevision,this._refreshCard()}));return}t.forEach(i=>{this._entityUnsubs.push(e.subscribeEntity(i,()=>{this._statesRevision=e.statesRevision,this._refreshCard()}))})}async updated(e){if(e.has("config")&&this.config){const t=JSON.stringify(this.config);if(t!==this._configKey)if(this._configKey=t,this._card&&this._lastType===this.config.type)this._card.setConfig(this.config);else{this._lastType=this.config.type;const i=await ul(this.config);i.flow=this.flow,i.preview=this.preview,this._card=i}}this._card&&(e.has("flow")||e.has("config")||e.has("preview"))&&(this._card.flow=this.flow,this._card.preview=this.preview),(e.has("flow")||e.has("config"))&&this._syncEntitySubscriptions(),this._card&&this._applyGridSize()}_applyGridSize(){const e=L_(this._card,this.config),t=Oh(e),{columnSize:i,rowSize:r,fullWidth:o,autoHeight:n}=M_(t);this.style.setProperty("--column-size",String(i)),this.style.setProperty("--row-size",String(r)),this.classList.toggle("full-width",o),this.classList.toggle("fit-rows",!n),this.toggleAttribute("auto-height",n)}render(){var r,o,n,s,a,l;if(!this._card)return m;const e=((r=this.config)==null?void 0:r.type)==="heading",t=((o=this.config)==null?void 0:o.type)==="line3d"||((n=this.config)==null?void 0:n.type)==="scatter3d"||((s=this.config)==null?void 0:s.type)==="line2d"||((a=this.config)==null?void 0:a.type)==="scatter2d"||((l=this.config)==null?void 0:l.type)==="bar";this._statesRevision;const i=u`
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
    `;return u`${i}`}};Fe.styles=$`
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
  `;bt([Tr({context:zi,subscribe:!0}),p({attribute:!1})],Fe.prototype,"flow",2);bt([p({attribute:!1})],Fe.prototype,"config",2);bt([p({type:Number})],Fe.prototype,"sectionIndex",2);bt([p({type:Number})],Fe.prototype,"cardIndex",2);bt([p({attribute:!1})],Fe.prototype,"sortableData",2);bt([p({type:Boolean})],Fe.prototype,"editMode",2);bt([p({type:Boolean})],Fe.prototype,"preview",2);bt([b()],Fe.prototype,"_card",2);bt([b()],Fe.prototype,"_statesRevision",2);Fe=bt([w("hui-card-container")],Fe);var X_=Object.defineProperty,Y_=Object.getOwnPropertyDescriptor,Ah=(e,t,i,r)=>{for(var o=r>1?void 0:r?Y_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&X_(t,i,o),o};let Vn=class extends y{updated(){const e=fd(this.background);if(!e){this.style.removeProperty("--section-background-color"),this.style.removeProperty("--section-background-opacity");return}const t=e.color&&e.color!=="default"?e.color:null;this.style.setProperty("--section-background-color",t?Xt(t):"var(--ha-section-background-color, var(--secondary-background-color))");const i=e.opacity!==void 0?e.opacity:_n;this.style.setProperty("--section-background-opacity",`${i}%`)}render(){return m}};Vn.styles=$`
    :host {
      position: absolute;
      inset: 0;
      border-radius: inherit;
      background-color: var(--section-background-color);
      opacity: var(--section-background-opacity, 50%);
      z-index: 0;
      pointer-events: none;
    }
  `;Ah([p({attribute:!1})],Vn.prototype,"background",2);Vn=Ah([w("flow-section-background")],Vn);var W_=Object.defineProperty,K_=Object.getOwnPropertyDescriptor,_t=(e,t,i,r)=>{for(var o=r>1?void 0:r?K_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&W_(t,i,o),o};const Z_={delay:100,delayOnTouchOnly:!0,direction:"vertical",invertedSwapThreshold:.7,group:"card"};let He=class extends y{constructor(){super(...arguments),this.viewIndex=0,this.sectionIndex=0,this.editMode=!1,this.preview=!1,this.layoutRevision=0,this._dragging=!1,this._cardConfigKeys=new WeakMap}_getCardKey(e){return this._cardConfigKeys.has(e)||this._cardConfigKeys.set(e,Math.random().toString(36)),this._cardConfigKeys.get(e)}_addCard(){this.dispatchEvent(new CustomEvent("add-card",{bubbles:!0,composed:!0,detail:{section:this.config}}))}_notifyLayoutChange(){this.dispatchEvent(new CustomEvent("layout-will-change",{bubbles:!0,composed:!0})),this.dispatchEvent(new CustomEvent("layout-changed",{bubbles:!0,composed:!0})),this.requestUpdate()}_cardMoved(e){if(e.stopPropagation(),!this.view)return;const{oldIndex:t,newIndex:i}=e.detail;t!==i&&(uc(this.view,this.sectionIndex,t,this.sectionIndex,i),this._notifyLayoutChange())}_cardAdded(e){if(e.stopPropagation(),!this.view)return;const t=wb(e.detail.data);t&&(t.sectionIndex===this.sectionIndex&&t.viewIndex===this.viewIndex||(uc(this.view,t.sectionIndex,t.cardIndex,this.sectionIndex,e.detail.index),this._notifyLayoutChange()))}_cardRemoved(e){e.stopPropagation(),e.detail.index}_dragStart(){this._dragging=!0}_dragEnd(){this._dragging=!1}render(){if(!this.config)return m;const e=this.config.cards,t=fd(this.config.background)!==void 0,i=u`
      <div class="card-grid ${this.editMode?"edit-mode":""} ${this._dragging?"dragging":""}">
        ${ph(e,(r,o)=>`${this.layoutRevision}:${o}:${this._getCardKey(r)}`,(r,o)=>u`
            <hui-card-container
              .sortableData=${[this.viewIndex,this.sectionIndex,o]}
              .config=${r}
              .sectionIndex=${this.sectionIndex}
              .cardIndex=${o}
              .editMode=${this.editMode}
              .preview=${this.preview}
            ></hui-card-container>
          `)}
        ${this.editMode?u`
              <button type="button" class="add-card" @click=${this._addCard}>
                <span class="add-plus">+</span>
              </button>
            `:m}
      </div>
    `;return u`
      <div class="section-root ${this.editMode?"edit-mode":""}">
        <div
          class="section-container ${t?"has-background":""}"
        >
          ${t?u`
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
              .options=${Z_}
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
    `}};He.styles=$`
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
  `;_t([Tr({context:zi,subscribe:!0}),p({attribute:!1})],He.prototype,"flow",2);_t([p({attribute:!1})],He.prototype,"config",2);_t([p({attribute:!1})],He.prototype,"view",2);_t([p({type:Number})],He.prototype,"viewIndex",2);_t([p({type:Number})],He.prototype,"sectionIndex",2);_t([p({type:Boolean})],He.prototype,"editMode",2);_t([p({type:Boolean})],He.prototype,"preview",2);_t([p({type:Number})],He.prototype,"layoutRevision",2);_t([b()],He.prototype,"_dragging",2);He=_t([w("flow-section")],He);var Q_=Object.defineProperty,J_=Object.getOwnPropertyDescriptor,gl=(e,t,i,r)=>{for(var o=r>1?void 0:r?J_(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Q_(t,i,o),o};let So=class extends y{constructor(){super(...arguments),this.sectionIndex=0,this.inactive=!1}connectedCallback(){super.connectedCallback(),this._unsubMenuOpen=pl(e=>{this.contains(e)||this.toggleAttribute("menu-open",!1)})}disconnectedCallback(){var e;(e=this._unsubMenuOpen)==null||e.call(this),super.disconnectedCallback()}_path(){return{sectionIndex:this.sectionIndex}}_menuItems(){return[{value:"edit",label:"编辑",icon:"mdi:pencil"},{value:"duplicate",label:"复制",icon:"mdi:content-copy"},{value:"divider",label:"",divider:!0},{value:"delete",label:"删除",icon:"mdi:delete",destructive:!0}]}_onMenuAction(e){const t=this._path();switch(e.detail.value){case"edit":Z(this,"ll-edit-section",t);break;case"duplicate":Z(this,"ll-duplicate-section",t);break;case"delete":Z(this,"ll-delete-section",t);break}}render(){return u`
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
    `}};So.styles=$`
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
  `;gl([p({type:Number})],So.prototype,"sectionIndex",2);gl([p({type:Boolean,reflect:!0})],So.prototype,"inactive",2);So=gl([w("hui-section-edit-mode")],So);var ey=Object.defineProperty,ty=Object.getOwnPropertyDescriptor,ri=(e,t,i,r)=>{for(var o=r>1?void 0:r?ty(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ey(t,i,o),o};const iy={delay:100,delayOnTouchOnly:!0,animation:150};let pt=class extends y{constructor(){super(...arguments),this.viewIndex=0,this.editMode=!1,this.layoutRevision=0,this._maxColumns=1,this._dragging=!1,this._sectionConfigKeys=new WeakMap}_getSectionKey(e){return this._sectionConfigKeys.has(e)||this._sectionConfigKeys.set(e,Math.random().toString(36)),this._sectionConfigKeys.get(e)}connectedCallback(){super.connectedCallback(),this._resizeObserver=new ResizeObserver(e=>{var i;const t=((i=e[0])==null?void 0:i.contentRect.width)??0;t&&this._updateMaxColumns(t)}),this._resizeObserver.observe(this)}disconnectedCallback(){var e;(e=this._resizeObserver)==null||e.disconnect(),super.disconnectedCallback()}_updateMaxColumns(e){var c;const t=getComputedStyle(this),i=sn(t.getPropertyValue("--column-min-width"))||320,r=sn(t.getPropertyValue("--column-gap"))||32,o=this.renderRoot.querySelector(".wrapper"),n=o?getComputedStyle(o):null,s=n?sn(n.paddingLeft)+sn(n.paddingRight):r*2,a=((c=this.config)==null?void 0:c.max_columns)??fh,l=Jb(e,{minColumnWidth:i,columnGap:r,horizontalPadding:s,configMaxColumns:a});l!==this._maxColumns&&(this._maxColumns=l)}_addSection(){this.dispatchEvent(new CustomEvent("add-section",{bubbles:!0,composed:!0}))}_sectionMoved(e){if(e.stopPropagation(),!this.config)return;const{oldIndex:t,newIndex:i}=e.detail;t!==i&&(this.dispatchEvent(new CustomEvent("layout-will-change",{bubbles:!0,composed:!0})),$b(this.config,t,i),this.dispatchEvent(new CustomEvent("layout-changed",{bubbles:!0,composed:!0})),this.requestUpdate())}_dragStart(){this._dragging=!0}_dragEnd(){this._dragging=!1}render(){if(!this.config)return m;const e=this.config.sections,t=Qb(e),i=e_({sectionColumnSpanSum:t,maxColumns:this._maxColumns,editMode:this.editMode}),r=!!this.config.dense_section_placement,o=u`
      <div class="container ${this._dragging?"dragging":""} ${r?"dense":""}">
        ${ph(e,n=>this._getSectionKey(n),(n,s)=>{const a=t_(n.column_span,i),l=n.row_span??1;return u`
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
        ${this.editMode?u`
              <button type="button" class="create-section" @click=${this._addSection}>
                <span class="create-icon">▦+</span>
                <span>拖放卡片到此处以创建新分区</span>
              </button>
            `:m}
      </div>
    `;return u`
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
          .options=${iy}
          @item-moved=${this._sectionMoved}
          @drag-start=${this._dragStart}
          @drag-end=${this._dragEnd}
        >
          ${o}
        </ha-sortable>
      </div>
    `}};pt.styles=$`
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
  `;ri([Tr({context:zi,subscribe:!0}),p({attribute:!1})],pt.prototype,"flow",2);ri([p({attribute:!1})],pt.prototype,"config",2);ri([p({type:Number})],pt.prototype,"viewIndex",2);ri([p({type:Boolean})],pt.prototype,"editMode",2);ri([p({type:Number})],pt.prototype,"layoutRevision",2);ri([b()],pt.prototype,"_maxColumns",2);ri([b()],pt.prototype,"_dragging",2);pt=ri([w("flow-view")],pt);var ry=Object.defineProperty,oy=Object.getOwnPropertyDescriptor,ms=(e,t,i,r)=>{for(var o=r>1?void 0:r?oy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ry(t,i,o),o};let wr=class extends y{constructor(){super(...arguments),this.views=[],this.activeIndex=0,this.editMode=!1}_select(e){e!==this.activeIndex&&this.dispatchEvent(new CustomEvent("view-selected",{bubbles:!0,composed:!0,detail:{index:e,path:na(this.views[e],e)}}))}_add(){this.dispatchEvent(new CustomEvent("view-add",{bubbles:!0,composed:!0}))}_edit(e,t){t.stopPropagation(),this.dispatchEvent(new CustomEvent("view-edit",{bubbles:!0,composed:!0,detail:{index:e}}))}_delete(e,t){t.stopPropagation(),this.dispatchEvent(new CustomEvent("view-delete",{bubbles:!0,composed:!0,detail:{index:e}}))}render(){return this.views.length?u`
      <div class="tabs" role="tablist">
        <div class="scroll">
          ${this.views.map((e,t)=>{const i=t===this.activeIndex,r=e.title||e.path||`视图 ${t+1}`,o=e.icon||"mdi:view-dashboard";return u`
              <button
                type="button"
                role="tab"
                class="tab ${i?"active":""}"
                aria-selected=${i?"true":"false"}
                title=${r}
                data-path=${na(e,t)}
                @click=${()=>this._select(t)}
                @dblclick=${n=>{this.editMode&&this._edit(t,n)}}
              >
                <ha-icon class="tab-icon" .icon=${o}></ha-icon>
                <span class="tab-label">${r}</span>
                ${this.editMode&&i?u`
                      <span
                        class="tab-edit"
                        title="编辑视图"
                        role="button"
                        tabindex="0"
                        @click=${n=>this._edit(t,n)}
                        @keydown=${n=>{(n.key==="Enter"||n.key===" ")&&(n.preventDefault(),this._edit(t,n))}}
                      >
                        <ha-svg-icon .path=${ba}></ha-svg-icon>
                      </span>
                    `:m}
                ${this.editMode&&this.views.length>1?u`
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
        ${this.editMode?u`
              <button
                type="button"
                class="add"
                title="添加视图"
                aria-label="添加视图"
                @click=${this._add}
              >
                <ha-svg-icon .path=${sp}></ha-svg-icon>
              </button>
            `:m}
      </div>
    `:m}};wr.styles=$`
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
  `;ms([p({attribute:!1})],wr.prototype,"views",2);ms([p({type:Number})],wr.prototype,"activeIndex",2);ms([p({type:Boolean})],wr.prototype,"editMode",2);wr=ms([w("flow-view-tabs")],wr);function Mr(e){let t=!1;return{onBackdropPointerDown:n=>{t=n.target===n.currentTarget},onBackdropPointerUp:n=>{t&&n.target===n.currentTarget&&e(),t=!1},onBackdropPointerCancel:()=>{t=!1}}}function hn(e,t=0){if(e==null||e==="")return t;const i=Number(e);return Number.isFinite(i)?i:t}function lo(e){if(Array.isArray(e)&&e.length>=1){const t=hn(e[0]),i=hn(e[1]);return e.slice(0,2).some(r=>r!=null&&r!==""&&Number.isFinite(Number(r)))?[t,i]:null}if(e&&typeof e=="object"){const t=e,i=t.default??t.values;return Array.isArray(i)&&i.length>=1?lo(i):t.x!==void 0||t.X!==void 0||t.y!==void 0||t.Y!==void 0?[hn(t.x??t.X),hn(t.y??t.Y)]:null}return null}function qn(e){if(Array.isArray(e))return e.map(lo).filter(t=>!!t);if(typeof e=="string"){const t=e.trim();if(!t)return[];try{return qn(JSON.parse(t))}catch{return[]}}return[]}function Gi(e,t){const i=Number.isFinite(t)&&t>0?Math.floor(t):5e3;return e.length>i?e.slice(-i):e}function Dh(e,t,i){if(t==null)return e;let r=t;if(typeof t=="string"){const o=t.trim();if(!o)return e;if(o.toLowerCase()==="clear")return[];try{r=JSON.parse(o)}catch{const n=o.split(/[,;\s]+/).filter(a=>a.length>0),s=lo(n);return s?Gi([...e,s],i):e}}if(r==="clear")return[];if(Array.isArray(r)){if(r.length>=1&&r.length<=3&&r.every(o=>typeof o=="number"||typeof o=="string")){const o=lo(r);return o?Gi([...e,o],i):e}return Gi(qn(r),i)}if(r&&typeof r=="object"){const o=r;if(o.clear===!0)return[];if(Array.isArray(o.points)){const s=qn(o.points);return o.append===!0||o.mode==="append"?Gi([...e,...s],i):Gi(s,i)}if(o.value!==void 0&&o.x===void 0&&o.X===void 0)return Dh(e,o.value,i);const n=lo(o);if(n)return Gi([...e,n],i)}return e}function ny(e=80){const t=[];for(let i=0;i<e;i+=1){const r=i/4;t.push([r,Math.sin(r/3)*5+Math.cos(r/7)*2])}return t}function sy(e=60){const t=[];for(let i=0;i<e;i+=1)t.push([(Math.random()-.5)*12,(Math.random()-.5)*12]);return t}class jw{constructor(){this.points=[],this._lastChanged=0,this._entity="",this._maxPoints=5e3}bind(t,i,r,o){if(this.dispose(),this._onChange=o,this._maxPoints=Number.isFinite(r)&&r>0?Math.floor(r):5e3,this._entity=i??"",!t||!i)return;let n=!0;this._unsub=t.subscribeEntity(i,s=>{s&&(this._consumeState(s,n),n=!1)})}dispose(){var t;(t=this._unsub)==null||t.call(this),this._unsub=void 0,this._lastChanged=0,this._entity="",this.points=[]}resetForEntity(t){t!==this._entity&&(this.points=[],this._lastChanged=0)}_consumeState(t,i){var o,n;if(t.last_changed<=this._lastChanged)return;i&&((o=t.attributes)!=null&&o.points)&&(this.points=qn(t.attributes.points));const r=t.state;r!=null&&r!==""&&(this.points=Dh(this.points,r,this._maxPoints)),this._lastChanged=t.last_changed,(n=this._onChange)==null||n.call(this)}}const Ie={bg_color:"",max_points:5e3,line_width:2,line_color:"#03a9f4",symbol_size:8,point_color:"#03a9f4",bar_color:"#03a9f4",x_label:"X",y_label:"Y",show_axes:!0,show_grid:!0};function Xi(e,t=0){if(e==null||e==="")return t;const i=Number(e);return Number.isFinite(i)?i:t}function co(e){if(Array.isArray(e)&&e.length>=1){const t=Xi(e[0]),i=Xi(e[1]),r=Xi(e[2]);return e.slice(0,3).some(o=>o!=null&&o!==""&&Number.isFinite(Number(o)))?[t,i,r]:null}if(e&&typeof e=="object"){const t=e,i=t.default??t.values;return Array.isArray(i)&&i.length>=1?co(i):t.x!==void 0||t.X!==void 0||t.y!==void 0||t.Y!==void 0||t.z!==void 0||t.Z!==void 0?[Xi(t.x??t.X),Xi(t.y??t.Y),Xi(t.z??t.Z)]:null}return null}function Gn(e){if(Array.isArray(e))return e.map(co).filter(t=>!!t);if(typeof e=="string"){const t=e.trim();if(!t)return[];try{return Gn(JSON.parse(t))}catch{return[]}}return[]}function Yi(e,t){const i=Number.isFinite(t)&&t>0?Math.floor(t):5e3;return e.length>i?e.slice(-i):e}function Ih(e,t,i){if(t==null)return e;let r=t;if(typeof t=="string"){const o=t.trim();if(!o)return e;if(o.toLowerCase()==="clear")return[];try{r=JSON.parse(o)}catch{const n=o.split(/[,;\s]+/).filter(a=>a.length>0),s=co(n);return s?Yi([...e,s],i):e}}if(r==="clear")return[];if(Array.isArray(r)){if(r.length>=1&&r.length<=4&&r.every(o=>typeof o=="number"||typeof o=="string")){const o=co(r);return o?Yi([...e,o],i):e}return Yi(Gn(r),i)}if(r&&typeof r=="object"){const o=r;if(o.clear===!0)return[];if(Array.isArray(o.points)){const s=Gn(o.points);return o.append===!0||o.mode==="append"?Yi([...e,...s],i):Yi(s,i)}if(o.value!==void 0&&o.x===void 0&&o.X===void 0)return Ih(e,o.value,i);const n=co(o);if(n)return Yi([...e,n],i)}return e}function ay(e=120){const t=[];for(let i=0;i<e;i+=1){const r=i/12;t.push([Math.cos(r)*6,Math.sin(r)*6,r*.8])}return t}function ly(e=80){const t=[];for(let i=0;i<e;i+=1)t.push([(Math.random()-.5)*12,(Math.random()-.5)*12,(Math.random()-.5)*12]);return t}class Vw{constructor(){this.points=[],this._lastChanged=0,this._entity="",this._maxPoints=5e3}bind(t,i,r,o){if(this.dispose(),this._onChange=o,this._maxPoints=Number.isFinite(r)&&r>0?Math.floor(r):5e3,this._entity=i??"",!t||!i)return;let n=!0;this._unsub=t.subscribeEntity(i,s=>{s&&(this._consumeState(s,n),n=!1)})}dispose(){var t;(t=this._unsub)==null||t.call(this),this._unsub=void 0,this._lastChanged=0,this._entity="",this.points=[]}resetForEntity(t){t!==this._entity&&(this.points=[],this._lastChanged=0)}_consumeState(t,i){var o,n;if(t.last_changed<=this._lastChanged)return;i&&((o=t.attributes)!=null&&o.points)&&(this.points=Gn(t.attributes.points));const r=t.state;r!=null&&r!==""&&(this.points=Ih(this.points,r,this._maxPoints)),this._lastChanged=t.last_changed,(n=this._onChange)==null||n.call(this)}}const Xe={bg_color:"",max_points:5e3,line_width:3,line_color:"#03a9f4",symbol_size:8,point_color:"#03a9f4",x_label:"X",y_label:"Y",z_label:"Z",show_axes:!0,auto_rotate:!1},cy="data:image/svg+xml,"+encodeURIComponent(`<svg xmlns="http://www.w3.org/2000/svg" width="96" height="56" viewBox="0 0 96 56" fill="none">
      <rect x="10" y="12" width="76" height="32" rx="6" fill="#fff" stroke="#e0e0e0"/>
      <circle cx="28" cy="28" r="8" fill="#cfd8dc"/>
      <rect x="44" y="22" width="30" height="5" rx="2.5" fill="#cfd8dc"/>
      <rect x="44" y="31" width="20" height="5" rx="2.5" fill="#cfd8dc"/>
    </svg>`),dy="data:image/svg+xml,"+encodeURIComponent(`<svg xmlns="http://www.w3.org/2000/svg" width="96" height="56" viewBox="0 0 96 56" fill="none">
      <rect x="22" y="6" width="52" height="44" rx="6" fill="#fff" stroke="#e0e0e0"/>
      <circle cx="48" cy="20" r="7" fill="#cfd8dc"/>
      <rect x="34" y="32" width="28" height="4" rx="2" fill="#cfd8dc"/>
      <rect x="38" y="39" width="20" height="4" rx="2" fill="#cfd8dc"/>
    </svg>`),hy={name:"",label:"",type:"grid",gridVariant:"appearance",schema:[{name:"icon",label:"图标",type:"icon",optional:!0,compact:!0},{name:"color",label:"颜色",type:"color",compact:!0}]},ne={name:"content_layout",label:"内容布局",type:"select_box",options:[{value:"horizontal",label:"水平",image:cy},{value:"vertical",label:"垂直",image:dy}]},Oc={name:"content",label:"内容",type:"section",schema:[{name:"name",label:"名称",type:"text",optional:!0,helper:"留空则使用动作默认名称"},hy]},uy={name:"entity",label:"动作",type:"entity"};function ve(e=[]){const t=[...Oc.schema??[],...e];return[uy,{...Oc,schema:t}]}const Vs=[{name:"bg_color",label:"背景色",type:"color",optional:!0}],qs=[{name:"x_label",label:"X 轴标签",type:"text",optional:!0,placeholder:Ie.x_label},{name:"y_label",label:"Y 轴标签",type:"text",optional:!0,placeholder:Ie.y_label},{name:"show_axes",label:"显示坐标轴",type:"boolean"},{name:"show_grid",label:"显示网格线",type:"boolean"}],Ac=[{name:"max_points",label:"最大点数",type:"number",helper:"超出后丢弃最旧的数据点"}],Dc=[{name:"bg_color",label:"背景色",type:"color",optional:!0}],Ic=[{name:"x_label",label:"X 轴标签",type:"text",optional:!0,placeholder:Xe.x_label},{name:"y_label",label:"Y 轴标签",type:"text",optional:!0,placeholder:Xe.y_label},{name:"z_label",label:"Z 轴标签",type:"text",optional:!0,placeholder:Xe.z_label},{name:"show_axes",label:"显示坐标轴",type:"boolean"},{name:"auto_rotate",label:"自动旋转",type:"boolean"}],Mc=[{name:"max_points",label:"最大点数",type:"number",helper:"超出后丢弃最旧的数据点"}],py={heading:[{name:"heading_style",label:"样式",type:"select",options:[{value:"title",label:"Title"},{value:"subtitle",label:"副标题"}]},{name:"heading",label:"标题文字",type:"text",placeholder:"客厅"},{name:"icon",label:"图标",type:"icon",placeholder:"mdi:sofa",optional:!0},{name:"badges",label:"徽章",type:"badges"}],tile:[...ve([{name:"hide_state",label:"隐藏状态",type:"boolean"},ne])],sensor:[...ve([{name:"hide_state",label:"隐藏状态",type:"boolean"},ne]),{name:"unit",label:"单位",type:"text",optional:!0,placeholder:"°C"}],label:[...ve([{name:"hide_state",label:"隐藏状态",type:"boolean"},ne])],switch:[...ve([ne])],trigger:[...ve([ne])],slider:[...ve([ne]),{name:"range",label:"范围",type:"grid",schema:[{name:"min",label:"最小值",type:"number"},{name:"max",label:"最大值",type:"number"},{name:"step",label:"步进",type:"number"}]}],climate:[...ve([ne]),{name:"range",label:"范围",type:"grid",schema:[{name:"min",label:"最小值",type:"number"},{name:"max",label:"最大值",type:"number"},{name:"step",label:"步进",type:"number"}]},{name:"unit",label:"单位",type:"text",optional:!0,placeholder:"°C"}],gain:[...ve([ne]),{name:"range",label:"范围 (dB)",type:"grid",schema:[{name:"min",label:"最小值",type:"number"},{name:"max",label:"最大值",type:"number"},{name:"step",label:"步进",type:"number"}]}],rgba:[...ve([ne])],hsv:[...ve([ne])],color:[...ve([ne])],"multi-fader":[...ve([ne,{name:"hide_state",label:"隐藏状态",type:"boolean"}]),{name:"count",label:"维度数量",type:"number",helper:"推杆个数（1–16），输出对应长度的数组"},{name:"labels",label:"通道标签",type:"text",optional:!0,placeholder:"0,1,2,3",helper:"用逗号分隔，数量不足时从 0 自动补序号"},{name:"orientation",label:"推杆方向",type:"select",options:[{value:"vertical",label:"竖直"},{value:"horizontal",label:"水平"}]},{name:"range",label:"范围",type:"grid",schema:[{name:"min",label:"最小值",type:"number"},{name:"max",label:"最大值",type:"number"},{name:"step",label:"步进",type:"number"}]}],"xy-pad":[{name:"entity",label:"动作（向量）",type:"entity",optional:!0,helper:"写入 [x, y] 数组；若同时填写 X/Y 动作则优先使用分动作模式"},{name:"axes",label:"分动作（可选）",type:"grid",schema:[{name:"entity_x",label:"X 动作",type:"entity",optional:!0},{name:"entity_y",label:"Y 动作",type:"entity",optional:!0}]},{name:"content",label:"内容",type:"section",schema:[{name:"name",label:"名称",type:"text",optional:!0,helper:"留空则使用地址默认名称"},{name:"",label:"",type:"grid",gridVariant:"appearance",schema:[{name:"icon",label:"图标",type:"icon",optional:!0,compact:!0},{name:"color",label:"颜色",type:"color",compact:!0}]},ne,{name:"show_grid",label:"显示十字线",type:"boolean"},{name:"invert_y",label:"Y 轴向上为正",type:"boolean",helper:"开启后触控板顶部对应 y_max"},{name:"snap_center",label:"松手回中",type:"boolean"}]},{name:"range",label:"范围",type:"grid",schema:[{name:"x_min",label:"X 最小",type:"number"},{name:"x_max",label:"X 最大",type:"number"},{name:"y_min",label:"Y 最小",type:"number"},{name:"y_max",label:"Y 最大",type:"number"},{name:"step",label:"步进",type:"number"}]}],cover:[...ve([ne]),{name:"position_count",label:"档位数量",type:"number",helper:"默认生成 0、1、2… 档位；留空自定义 positions 时优先使用 positions"}],media:[...ve([ne]),{name:"play_value",label:"播放指令",type:"text",optional:!0,placeholder:"true",helper:"写入地址的值，支持 true / false / 数字 / 字符串"},{name:"stop_value",label:"停止指令",type:"text",optional:!0,placeholder:"false"}],clock:[{name:"title",label:"标题",type:"text",optional:!0},{name:"clock_style",label:"样式",type:"select",options:[{value:"digital",label:"数字"},{value:"analog",label:"模拟"}]},{name:"clock_size",label:"尺寸",type:"select",options:[{value:"small",label:"小"},{value:"medium",label:"中"},{value:"large",label:"大"}]},{name:"time_format",label:"时间格式",type:"select",options:[{value:"auto",label:"跟随系统"},{value:"24",label:"24 小时"},{value:"12",label:"12 小时"}],optional:!0},{name:"show_seconds",label:"显示秒",type:"boolean"},{name:"no_background",label:"无背景",type:"boolean"},{name:"time_zone",label:"时区",type:"text",optional:!0,placeholder:"Asia/Shanghai",helper:"留空则使用浏览器时区"},{name:"analog_border",label:"表盘边框",type:"boolean",visible:{field:"clock_style",operator:"eq",value:"analog"}},{name:"analog_face_style",label:"表盘数字",type:"select",options:[{value:"markers",label:"刻度"},{value:"numbers",label:"阿拉伯数字"},{value:"roman",label:"罗马数字"}],visible:{field:"clock_style",operator:"eq",value:"analog"}},{name:"analog_ticks",label:"刻度",type:"select",options:[{value:"hour",label:"小时"},{value:"quarter",label:"一刻钟"},{value:"minute",label:"分钟"},{value:"none",label:"无"}],visible:{field:"clock_style",operator:"eq",value:"analog"}}],markdown:[{name:"style",label:"样式",type:"select",options:[{value:"card",label:"卡片"},{value:"text-only",label:"纯文本"}]},{name:"title",label:"标题",type:"text",optional:!0,visible:{field:"style",operator:"not_eq",value:"text-only"}},{name:"content",label:"内容",type:"textarea",helper:"支持 Markdown 语法"}],link:[{name:"name",label:"名称",type:"text",placeholder:"文档"},{name:"url",label:"链接地址",type:"text",placeholder:"http://127.0.0.1:8992/"},{name:"icon",label:"图标",type:"icon",optional:!0,placeholder:"mdi:link-variant"},{name:"new_tab",label:"新标签页打开",type:"boolean"}],"picture-elements":[{name:"title",label:"标题",type:"text",optional:!0,placeholder:"户型图"},{name:"image",label:"底图",type:"image"},{name:"elements",label:"元素",type:"picture_elements"}],line3d:[{name:"entity",label:"动作",type:"entity"},{name:"name",label:"名称",type:"text",optional:!0},...Mc,{name:"line_width",label:"线宽",type:"number"},{name:"line_color",label:"线条颜色",type:"color",optional:!0},...Dc,...Ic],scatter3d:[{name:"entity",label:"动作",type:"entity"},{name:"name",label:"名称",type:"text",optional:!0},...Mc,{name:"symbol_size",label:"点大小",type:"number"},{name:"point_color",label:"点颜色",type:"color",optional:!0},...Dc,...Ic],line2d:[{name:"entity",label:"动作",type:"entity"},{name:"name",label:"名称",type:"text",optional:!0},...Ac,{name:"line_width",label:"线宽",type:"number"},{name:"line_color",label:"线条颜色",type:"color",optional:!0},...Vs,...qs],scatter2d:[{name:"entity",label:"动作",type:"entity"},{name:"name",label:"名称",type:"text",optional:!0},...Ac,{name:"symbol_size",label:"点大小",type:"number"},{name:"point_color",label:"点颜色",type:"color",optional:!0},...Vs,...qs],bar:[{name:"name",label:"名称",type:"text",optional:!0},{name:"entities",label:"柱",type:"bar_entities"},{name:"bar_color",label:"默认柱颜色",type:"color",optional:!0,helper:"未单独设色的柱使用此颜色"},...Vs,...qs],status:[{name:"name",label:"名称",type:"text",optional:!0,placeholder:"Flow 连接"},{name:"",label:"",type:"grid",gridVariant:"appearance",schema:[{name:"icon",label:"图标",type:"icon",optional:!0,compact:!0},{name:"color",label:"正常色",type:"color",compact:!0}]},ne,{name:"list_columns",label:"状态列数",type:"select",options:[{value:"1",label:"单列"},{value:"2",label:"双列"}]},{name:"entities",label:"状态项",type:"status_entities"}]};function fy(e){return py[e==="button"?"trigger":e==="color"?"rgba":e]??[{name:"entity",label:"动作",type:"entity",optional:!0},{name:"name",label:"名称",type:"text",optional:!0}]}function gy(e){const t=e.type,i=!!e.vertical,r={...e.grid_options??{}};if(t==="tile"||t==="trigger"||t==="sensor"||t==="label"){const o=i?2:1;e.grid_options={...r,columns:r.columns??6,rows:o,min_rows:o,min_columns:i?3:6};return}if(t==="multi-fader"){const o=Math.max(1,Math.min(16,Math.round(Number(e.count)||4))),n=e.orientation==="horizontal"?"horizontal":"vertical",s=n==="vertical"?4:Math.max(2,Math.min(8,1+Math.ceil(o/2)));e.grid_options={...r,columns:r.columns??(n==="vertical"?Math.min(12,Math.max(6,o*2)):6),rows:s,min_rows:n==="vertical"?3:s,min_columns:i?3:6};return}if(t==="xy-pad"||t==="rgba"||t==="hsv"||t==="color"){const o=t==="xy-pad"?i?4:3:5;e.grid_options={...r,columns:r.columns??6,rows:o,min_rows:o,min_columns:i?3:6};return}if(t==="slider"||t==="switch"||t==="climate"||t==="cover"||t==="gain"||t==="media"){const o=i?3:2;e.grid_options={...r,columns:r.columns??6,rows:o,min_rows:o,min_columns:i?3:6};return}t==="status"&&(e.grid_options={...r,columns:r.columns??(i?3:6),rows:"auto",min_rows:2,min_columns:i?3:6})}function my(e,t){const i={...e};i.type==="button"&&(i.type="trigger"),i.type==="color"&&(i.type="rgba"),i.content_layout!==void 0&&(i.vertical=i.content_layout==="vertical",delete i.content_layout),t!=null&&t.layoutChanged&&(i.type==="tile"||i.type==="trigger"||i.type==="sensor"||i.type==="label"||i.type==="slider"||i.type==="switch"||i.type==="climate"||i.type==="cover"||i.type==="gain"||i.type==="media"||i.type==="rgba"||i.type==="hsv"||i.type==="color"||i.type==="xy-pad"||i.type==="multi-fader"||i.type==="status")&&gy(i),i.style!==void 0&&(i.style==="text-only"?i.text_only=!0:delete i.text_only,delete i.style),i.time_format==="auto"&&delete i.time_format,i.color||delete i.color;const r=i.type==="markdown"?["range",""]:["content","range",""];for(const o of r)o in i&&delete i[o];return i.type==="status"&&(Number(i.list_columns)===2?i.list_columns=2:delete i.list_columns),i}function vy(e){const t={...e};return"vertical"in t?t.content_layout=t.vertical?"vertical":"horizontal":(t.type==="tile"||t.type==="trigger"||t.type==="slider"||t.type==="switch"||t.type==="climate"||t.type==="cover"||t.type==="gain"||t.type==="media"||t.type==="status")&&(t.content_layout="horizontal"),t.text_only?t.style="text-only":t.type==="markdown"&&(t.style="card"),t.type==="clock"&&!t.time_format&&(t.time_format="auto"),t.type==="climate"&&(t.min===void 0&&(t.min=0),t.max===void 0&&(t.max=100),t.step===void 0&&(t.step=1)),t.type==="gain"&&(t.min===void 0&&(t.min=-60),t.max===void 0&&(t.max=12),t.step===void 0&&(t.step=1)),t.type==="cover"&&t.position_count===void 0&&(t.position_count=4),t.type==="clock"&&(t.clock_style||(t.clock_style="digital"),t.clock_size||(t.clock_size="small")),t.type==="clock"&&t.analog_border===void 0&&t.clock_style==="analog"&&(t.analog_border=!0),t.type==="clock"&&!t.analog_face_style&&t.clock_style==="analog"&&(t.analog_face_style="markers"),t.type==="heading"&&(t.heading_style||(t.heading_style="title"),t.heading||(t.heading="新建部件"),t.badges||(t.badges=[])),!t.elements&&t.type==="picture-elements"&&(t.elements=[]),t.type==="line3d"&&(t.max_points===void 0&&(t.max_points=Xe.max_points),t.line_width===void 0&&(t.line_width=Xe.line_width),t.show_axes===void 0&&(t.show_axes=Xe.show_axes),t.auto_rotate===void 0&&(t.auto_rotate=Xe.auto_rotate)),t.type==="scatter3d"&&(t.max_points===void 0&&(t.max_points=Xe.max_points),t.symbol_size===void 0&&(t.symbol_size=Xe.symbol_size),t.show_axes===void 0&&(t.show_axes=Xe.show_axes),t.auto_rotate===void 0&&(t.auto_rotate=Xe.auto_rotate)),t.type==="line2d"&&(t.max_points===void 0&&(t.max_points=Ie.max_points),t.line_width===void 0&&(t.line_width=Ie.line_width),t.show_axes===void 0&&(t.show_axes=Ie.show_axes),t.show_grid===void 0&&(t.show_grid=Ie.show_grid)),t.type==="scatter2d"&&(t.max_points===void 0&&(t.max_points=Ie.max_points),t.symbol_size===void 0&&(t.symbol_size=Ie.symbol_size),t.show_axes===void 0&&(t.show_axes=Ie.show_axes),t.show_grid===void 0&&(t.show_grid=Ie.show_grid)),t.type==="bar"&&(t.entities||(t.entities=[]),t.show_axes===void 0&&(t.show_axes=Ie.show_axes),t.show_grid===void 0&&(t.show_grid=Ie.show_grid)),t.type==="status"&&(t.entities||(t.entities=[]),t.icon||(t.icon="mdi:lan-connect"),t.list_columns=Number(t.list_columns)===2?"2":"1"),t.type==="link"&&(t.new_tab===void 0&&(t.new_tab=!0),t.url||(t.url="http://127.0.0.1:8992/")),t}var by=Object.defineProperty,_y=Object.getOwnPropertyDescriptor,vs=(e,t,i,r)=>{for(var o=r>1?void 0:r?_y(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&by(t,i,o),o};let $r=class extends y{constructor(){super(...arguments),this.header="",this.expanded=!0,this._open=!0}willUpdate(e){e.has("expanded")&&(this._open=this.expanded)}_toggle(){this._open=!this._open}render(){return u`
      <div class="panel">
        <button type="button" class="header" @click=${this._toggle}>
          <span class="chevron ${O({open:this._open})}">›</span>
          <span class="title">${this.header}</span>
        </button>
        ${this._open?u`<div class="body"><slot></slot></div>`:""}
      </div>
    `}};$r.styles=$`
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
  `;vs([p()],$r.prototype,"header",2);vs([p({type:Boolean,reflect:!0})],$r.prototype,"expanded",2);vs([b()],$r.prototype,"_open",2);$r=vs([w("ha-expandable-panel")],$r);function nt(e){var o,n,s,a;const t=((o=e.name)==null?void 0:o.trim())||((n=e.suggestedName)==null?void 0:n.trim());if(t)return t.replace(/^\d+:\s*/,"");const i=(s=e.nodeName)==null?void 0:s.trim();if(i)return i;const r=(a=e.nodeType)==null?void 0:a.trim();return r||e.entity}function zc(e,t){const i=t.trim();if(i)return e.find(r=>r.entity===i)}function Nc(e,t){const i=t.trim().toLowerCase();return i?e.filter(r=>nt(r).toLowerCase().includes(i)):[...e]}async function yy(e,t){const i=e.trim();if(!i)return;const r=(t==null?void 0:t.trim())||i;try{const o=await fetch("/api/actions",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({entity:i,name:r})});if(!o.ok)return;const n=await o.json();return wy(),n.item}catch{return}}async function xy(){try{const e=await fetch("/api/actions");if(!e.ok)return[];const t=await e.json();return Array.isArray(t.items)?t.items:[]}catch{return[]}}function wy(){document.dispatchEvent(new CustomEvent("flow-actions-changed"))}var $y=Object.defineProperty,ky=Object.getOwnPropertyDescriptor,je=(e,t,i,r)=>{for(var o=r>1?void 0:r?ky(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&$y(t,i,o),o};let _e=class extends y{constructor(){super(...arguments),this.label="动作",this.value="",this.placeholder="搜索动作名",this.optional=!1,this.compact=!1,this._actions=[],this._open=!1,this._menuStyle={},this._focused=!1,this._query="",this._editedQuery=!1,this._suppressNextBlur=!1,this._onActionsChanged=()=>{this._loadActions()},this._onDocumentClick=e=>{if(!this._open)return;e.composedPath().includes(this)||this._close()},this._repositionMenu=()=>{this._open&&this._updateMenuPosition()}}connectedCallback(){super.connectedCallback(),document.addEventListener("click",this._onDocumentClick),document.addEventListener("flow-actions-changed",this._onActionsChanged),window.addEventListener("resize",this._repositionMenu),this._loadActions()}disconnectedCallback(){document.removeEventListener("click",this._onDocumentClick),document.removeEventListener("flow-actions-changed",this._onActionsChanged),window.removeEventListener("resize",this._repositionMenu),this._detachScrollListener(),super.disconnectedCallback()}async _loadActions(){const e=await xy();this._actions=e;const t=e.map(i=>i.entity).filter(Boolean);t.length&&this.flow&&this.flow.connection.trackAddresses(t)}_inputText(){if(this._focused)return this._query;const e=zc(this._actions,this.value);return e?nt(e):""}_actionOptions(){return[...Nc(this._actions,this._focused?this._query:"")].sort((t,i)=>t.used!==i.used?t.used?1:-1:nt(t).localeCompare(nt(i),"zh-CN")).map(t=>({action:t,label:nt(t),hint:t.entity}))}_emitSelection(e,t){this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:e,actionName:t}}))}_attachScrollListener(){this._detachScrollListener();let e=this.parentElement;for(;e;){const t=getComputedStyle(e);if(/(auto|scroll)/.test(t.overflowY)){this._scrollParent=e,e.addEventListener("scroll",this._repositionMenu,{passive:!0});return}e=e.parentElement}}_detachScrollListener(){var e;(e=this._scrollParent)==null||e.removeEventListener("scroll",this._repositionMenu),this._scrollParent=void 0}_updateMenuPosition(){var i;const e=(i=this.shadowRoot)==null?void 0:i.querySelector(".input-wrap");if(!e)return;const t=fl(e);this._menuStyle={position:"fixed",top:`${t.top}px`,left:`${t.left}px`,width:`${t.width}px`,maxHeight:`${t.maxHeight}px`,zIndex:"500"}}_openMenu(){this._open||(this._open=!0,this._attachScrollListener(),this.requestUpdate(),requestAnimationFrame(()=>this._updateMenuPosition()))}_close(){this._open&&(this._open=!1,this._detachScrollListener())}_onInput(e){this._editedQuery=!0,this._query=e.target.value,this._openMenu(),requestAnimationFrame(()=>this._updateMenuPosition())}_onFocus(){this._loadActions(),this._focused=!0,this._editedQuery=!1;const e=zc(this._actions,this.value);this._query=e?nt(e):"",this._openMenu()}_onBlur(){window.setTimeout(()=>{if(this._suppressNextBlur){this._suppressNextBlur=!1,this._focused=!1,this._query="",this._close();return}this._focused=!1;const e=this._query.trim();if(!e){this.optional&&this._editedQuery&&this._emitSelection(""),this._query="",this._close();return}const t=Nc(this._actions,e).find(i=>nt(i).toLowerCase()===e.toLowerCase());t&&t.entity!==this.value&&this._emitSelection(t.entity,nt(t)),this._query="",this._close()},0)}_selectAction(e){this._suppressNextBlur=!0,this._focused=!1,this._editedQuery=!1,this._query="",this._close(),this._emitSelection(e.entity,nt(e))}_renderOption(e){return u`
      <button
        type="button"
        class="option"
        @mousedown=${t=>t.preventDefault()}
        @click=${()=>this._selectAction(e.action)}
      >
        <span class="option-value">${e.label}</span>
        ${e.hint?u`<span class="option-hint">${e.hint}</span>`:m}
      </button>
    `}render(){const e=this._actionOptions(),t=e.length>0;return u`
      <label class="field ${this.compact?"compact":""}">
        ${this.compact?m:u`
              <span class="label">
                ${this.label}
                ${this.optional?u`<span class="optional">（可选）</span>`:""}
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
      ${this._open&&t?u`
            <div class="menu" style=${T(this._menuStyle)}>
              <div class="section">
                <div class="section-title">动作库</div>
                ${e.map(i=>this._renderOption(i))}
              </div>
            </div>
          `:this._open?u`
              <div class="menu" style=${T(this._menuStyle)}>
                <div class="section empty-hint">
                  <p>未找到匹配动作，请先在设置页添加到动作库</p>
                </div>
              </div>
            `:m}
    `}};_e.styles=$`
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
  `;je([Tr({context:zi,subscribe:!0}),p({attribute:!1})],_e.prototype,"flow",2);je([p()],_e.prototype,"label",2);je([p()],_e.prototype,"value",2);je([p()],_e.prototype,"placeholder",2);je([p({type:Boolean})],_e.prototype,"optional",2);je([p({type:Boolean,reflect:!0})],_e.prototype,"compact",2);je([b()],_e.prototype,"_actions",2);je([b()],_e.prototype,"_open",2);je([b()],_e.prototype,"_menuStyle",2);je([b()],_e.prototype,"_focused",2);je([b()],_e.prototype,"_query",2);_e=je([w("ha-entity-picker")],_e);var Sy=Object.defineProperty,Cy=Object.getOwnPropertyDescriptor,zr=(e,t,i,r)=>{for(var o=r>1?void 0:r?Cy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Sy(t,i,o),o};let Kt=class extends y{constructor(){super(...arguments),this.label="",this.hasValue=!1,this.disabled=!1,this.opened=!1,this.compact=!1}render(){const e=this.hasValue&&this.label?u`<span class="overline">${this.label}</span>`:m,t=this.hasValue?u`<span class="headline"><slot name="value"></slot></span>`:u`<span class="headline placeholder">${this.label}</span>`;return u`
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
    `}};Kt.styles=$`
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
  `;zr([p()],Kt.prototype,"label",2);zr([p({type:Boolean})],Kt.prototype,"hasValue",2);zr([p({type:Boolean})],Kt.prototype,"disabled",2);zr([p({type:Boolean,reflect:!0})],Kt.prototype,"opened",2);zr([p({type:Boolean,reflect:!0})],Kt.prototype,"compact",2);Kt=zr([w("ha-picker-field")],Kt);var Ey=Object.defineProperty,Py=Object.getOwnPropertyDescriptor,yt=(e,t,i,r)=>{for(var o=r>1?void 0:r?Py(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ey(t,i,o),o};let Ue=class extends y{constructor(){super(...arguments),this.label="图标",this.value="",this.placeholder="mdi:home",this.compact=!1,this._open=!1,this._filter="",this._results=[],this._registryReady=!1,this._menuStyle={},this._onDocumentClick=e=>{if(!this._open)return;e.composedPath().includes(this)||this._close()},this._repositionMenu=()=>{this._open&&this._updateMenuPosition()}}connectedCallback(){super.connectedCallback(),document.addEventListener("click",this._onDocumentClick),window.addEventListener("resize",this._repositionMenu),this._results=Jo(""),this._unloadRegistryListener=vd(()=>{this._registryReady=!0,this._results=Jo(this._filter),this.requestUpdate()}),go()}disconnectedCallback(){var e;document.removeEventListener("click",this._onDocumentClick),window.removeEventListener("resize",this._repositionMenu),this._detachScrollListener(),(e=this._unloadRegistryListener)==null||e.call(this),super.disconnectedCallback()}_attachScrollListener(){this._detachScrollListener();let e=this.parentElement;for(;e;){const t=getComputedStyle(e);if(/(auto|scroll)/.test(t.overflowY)){this._scrollParent=e,e.addEventListener("scroll",this._repositionMenu,{passive:!0});return}e=e.parentElement}}_detachScrollListener(){var e;(e=this._scrollParent)==null||e.removeEventListener("scroll",this._repositionMenu),this._scrollParent=void 0}_updateMenuPosition(){var i;const e=(i=this.shadowRoot)==null?void 0:i.querySelector("ha-picker-field");if(!e)return;const t=fl(e);this._menuStyle={position:"fixed",top:`${t.top}px`,left:`${t.left}px`,width:`${t.width}px`,maxHeight:`${t.maxHeight}px`,zIndex:"500"}}_toggleOpen(e){if(e.stopPropagation(),this._open){this._close();return}go().then(()=>{this._filter="",this._results=Jo(""),this._open=!0,this._attachScrollListener(),this.requestUpdate(),requestAnimationFrame(()=>{var t,i;this._updateMenuPosition(),(i=(t=this.shadowRoot)==null?void 0:t.querySelector(".search-input"))==null||i.focus()})})}_close(){this._open=!1,this._detachScrollListener()}_onFilterInput(e){const t=e.target.value;this._filter=t,this._results=Jo(t)}_select(e,t){t.stopPropagation(),this._emit(e),this._close(),this._filter=""}_emit(e){this.dispatchEvent(new CustomEvent("icon-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}render(){const e=this.value||this.placeholder,t=mi(e),i=!!this.value;return u`
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
        ${this._open?u`
              <div class="menu" style=${T(this._menuStyle)} @click=${r=>r.stopPropagation()}>
                <div class="search">
                  <ha-svg-icon class="search-icon" .path=${mi("mdi:magnify")}></ha-svg-icon>
                  <input
                    class="search-input"
                    placeholder="搜索 | 添加自定义项目"
                    .value=${this._filter}
                    @input=${this._onFilterInput}
                  />
                </div>
                <ul class="list" role="listbox">
                  ${this._registryReady?this._results.length?this._results.map(r=>u`
                          <li>
                            <button
                              type="button"
                              class="option ${r===this.value?"selected":""}"
                              @click=${o=>this._select(r,o)}
                            >
                              <ha-svg-icon .path=${mi(r)}></ha-svg-icon>
                              <span>${r}</span>
                            </button>
                          </li>
                        `):u`<li class="empty">未找到匹配的图标</li>`:u`<li class="empty">正在加载图标库…</li>`}
                </ul>
              </div>
            `:m}
      </div>
    `}};Ue.styles=$`
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
  `;yt([p()],Ue.prototype,"label",2);yt([p()],Ue.prototype,"value",2);yt([p()],Ue.prototype,"placeholder",2);yt([p({type:Boolean})],Ue.prototype,"compact",2);yt([b()],Ue.prototype,"_open",2);yt([b()],Ue.prototype,"_filter",2);yt([b()],Ue.prototype,"_results",2);yt([b()],Ue.prototype,"_registryReady",2);yt([b()],Ue.prototype,"_menuStyle",2);Ue=yt([w("ha-icon-picker")],Ue);var Ty=Object.defineProperty,Oy=Object.getOwnPropertyDescriptor,Oe=(e,t,i,r)=>{for(var o=r>1?void 0:r?Oy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ty(t,i,o),o};const Rc={label:"状态颜色"};let he=class extends y{constructor(){super(...arguments),this.label="颜色",this.value="",this.compact=!1,this.hideHelper=!1,this.includeState=!0,this.includeDefault=!1,this.defaultLabel="默认",this.helper="",this._open=!1,this._custom="#03a9f4",this._showCustom=!1,this._menuStyle={},this._onDocumentClick=e=>{this._open&&(e.composedPath().includes(this)||this._close())},this._repositionMenu=()=>{this._open&&this._updateMenuPosition()}}connectedCallback(){super.connectedCallback(),document.addEventListener("click",this._onDocumentClick),window.addEventListener("resize",this._repositionMenu),this._syncCustom()}disconnectedCallback(){document.removeEventListener("click",this._onDocumentClick),window.removeEventListener("resize",this._repositionMenu),this._detachScrollListener(),super.disconnectedCallback()}updated(e){e.has("value")&&this._syncCustom()}_syncCustom(){this.value&&!io.some(e=>e.value===this.value)&&this.value!==""&&(this._custom=this.value)}_attachScrollListener(){this._detachScrollListener();let e=this.parentElement;for(;e;){const t=getComputedStyle(e);if(/(auto|scroll)/.test(t.overflowY)){this._scrollParent=e,e.addEventListener("scroll",this._repositionMenu,{passive:!0});return}e=e.parentElement}}_detachScrollListener(){var e;(e=this._scrollParent)==null||e.removeEventListener("scroll",this._repositionMenu),this._scrollParent=void 0}_updateMenuPosition(){var i;const e=(i=this.shadowRoot)==null?void 0:i.querySelector("ha-picker-field");if(!e)return;const t=fl(e);this._menuStyle={position:"fixed",top:`${t.top}px`,left:`${t.left}px`,width:`${t.width}px`,maxHeight:`${t.maxHeight}px`,zIndex:"500"}}_displayLabel(){return this.value?Jp(this.value):this.includeDefault?this.defaultLabel:Rc.label}_emptySelected(){return!this.value}_showPrimaryOption(){return this.includeState||this.includeDefault}_primaryOptionLabel(){return this.includeDefault?this.defaultLabel:Rc.label}_displayColor(){return this.value?Xt(this.value):"var(--primary-color, #03a9f4)"}_emit(e){this.dispatchEvent(new CustomEvent("color-changed",{bubbles:!0,composed:!0,detail:{value:e}}))}_select(e){this._showCustom=!1,this._close(),this._emit(e)}_applyCustom(){this._close(),this._emit(this._custom)}_toggleOpen(e){if(e.stopPropagation(),this._open){this._close();return}this._open=!0,this._showCustom=!1,this._attachScrollListener(),requestAnimationFrame(()=>this._updateMenuPosition())}_close(){this._open=!1,this._detachScrollListener()}_renderSwatch(e,t=!1){return t?u`<span class="swatch state"></span>`:u`<span class="swatch" style=${`background:${e}`}></span>`}render(){const e=mi("mdi:palette"),t=!!this.value,i=this._displayColor(),r=t||this.compact||this.includeDefault&&!this.value;return u`
      <div class="picker ${this.compact?"compact":""}">
        <ha-picker-field
          .label=${this.label}
          .hasValue=${r}
          .opened=${this._open}
          .compact=${this.compact}
          @click=${this._toggleOpen}
        >
          ${t&&!io.some(o=>o.value===this.value)?u`<span slot="leading" class="swatch leading" style=${`background:${i}`}></span>`:u`<ha-svg-icon slot="leading" .path=${e}></ha-svg-icon>`}
          <span slot="value">${this._displayLabel()}</span>
        </ha-picker-field>

        ${this._open?u`
              <div class="menu" style=${T(this._menuStyle)} @click=${o=>o.stopPropagation()}>
                ${this._showPrimaryOption()?u`
                      <button
                        type="button"
                        class="option ${this._emptySelected()?"selected":""}"
                        @click=${()=>this._select("")}
                      >
                        ${this.includeDefault?u`<span class="swatch default"></span>`:this._renderSwatch("var(--primary-color)",!0)}
                        <span>${this._primaryOptionLabel()}</span>
                      </button>
                    `:m}
                ${io.map(o=>u`
                    <button
                      type="button"
                      class="option ${this.value===o.value?"selected":""}"
                      @click=${()=>this._select(o.value)}
                    >
                      ${this._renderSwatch(Xt(o.value))}
                      <span>${o.label}</span>
                    </button>
                  `)}
                <div class="custom-section">
                  <button
                    type="button"
                    class="option custom-toggle"
                    @click=${()=>{this._showCustom=!this._showCustom}}
                  >
                    <ha-svg-icon class="rainbow" .path=${mi("mdi:palette-swatch")}></ha-svg-icon>
                    <span>自定义颜色</span>
                  </button>
                  ${this._showCustom?u`
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

        ${this.hideHelper?m:u`<p class="helper">${this.helper||"非活动状态（例如关闭或闭合）将不会被着色。"}</p>`}
      </div>
    `}};he.styles=$`
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
  `;Oe([p()],he.prototype,"label",2);Oe([p()],he.prototype,"value",2);Oe([p({type:Boolean})],he.prototype,"compact",2);Oe([p({type:Boolean})],he.prototype,"hideHelper",2);Oe([p({type:Boolean})],he.prototype,"includeState",2);Oe([p({type:Boolean})],he.prototype,"includeDefault",2);Oe([p()],he.prototype,"defaultLabel",2);Oe([p()],he.prototype,"helper",2);Oe([b()],he.prototype,"_open",2);Oe([b()],he.prototype,"_custom",2);Oe([b()],he.prototype,"_showCustom",2);Oe([b()],he.prototype,"_menuStyle",2);he=Oe([w("ha-color-picker")],he);var Ay=Object.defineProperty,Dy=Object.getOwnPropertyDescriptor,Nr=(e,t,i,r)=>{for(var o=r>1?void 0:r?Dy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ay(t,i,o),o};let Zt=class extends y{constructor(){super(...arguments),this.options=[],this.disabled=!1,this.maxColumns=3}_select(e){this.disabled||e===(this.value??"")||this.dispatchEvent(new CustomEvent("value-changed",{detail:{value:e},bubbles:!0,composed:!0}))}render(){const e=Math.min(this.maxColumns,Math.max(this.options.length,1));return u`
      ${this.label?u`<div class="label">${this.label}</div>`:m}
      <div class="list" style="--columns: ${e}">
        ${this.options.map(t=>{const i=t.value===this.value,r=!!(t.disabled||this.disabled);return u`
            <button
              type="button"
              class=${O({option:!0,selected:i})}
              ?disabled=${r}
              @click=${()=>this._select(t.value)}
            >
              <div class="content">
                <span class=${O({radio:!0,checked:i})}></span>
                <div class="text">
                  <span class="option-label">${t.label}</span>
                  ${t.description?u`<span class="description">${t.description}</span>`:m}
                </div>
              </div>
              ${t.image?u`<img class="preview" src=${t.image} alt="" draggable="false" />`:m}
            </button>
          `})}
      </div>
    `}};Zt.styles=$`
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
  `;Nr([p({attribute:!1})],Zt.prototype,"options",2);Nr([p()],Zt.prototype,"value",2);Nr([p()],Zt.prototype,"label",2);Nr([p({type:Boolean})],Zt.prototype,"disabled",2);Nr([p({type:Number,attribute:"max_columns"})],Zt.prototype,"maxColumns",2);Zt=Nr([w("ha-select-box")],Zt);var Iy=Object.defineProperty,My=Object.getOwnPropertyDescriptor,Mh=(e,t,i,r)=>{for(var o=r>1?void 0:r?My(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Iy(t,i,o),o};let Xn=class extends y{constructor(){super(...arguments),this.badges=[]}_emit(e){this.dispatchEvent(new CustomEvent("badges-changed",{bubbles:!0,composed:!0,detail:{badges:e}}))}_add(e){const t=e==="button"?{type:"button",entity:"",icon:"mdi:gesture-tap-button",text:"触发",value:!0,show_name:!0}:{type:"entity",entity:"",icon:"mdi:thermometer",show_icon:!0,show_name:!0};this._emit([...this.badges,t])}_remove(e){this._emit(this.badges.filter((t,i)=>i!==e))}_patch(e,t){const i=this.badges.map((r,o)=>o===e?{...r,...t}:r);this._emit(i)}_changeType(e,t){const i=this.badges[e];if(t==="button"){this._patch(e,{type:"button",text:String((i==null?void 0:i.text)??(i==null?void 0:i.name)??"触发"),value:(i==null?void 0:i.value)??!0,icon:(i==null?void 0:i.icon)||"mdi:gesture-tap-button",show_name:(i==null?void 0:i.show_name)??!0});return}this._patch(e,{type:"entity",show_icon:(i==null?void 0:i.show_icon)??!0,show_name:(i==null?void 0:i.show_name)??(i==null?void 0:i.show_state)??!0,icon:(i==null?void 0:i.icon)||"mdi:thermometer"})}render(){return u`
      <div class="badges-editor">
        <div class="head">
          <span class="label">徽章</span>
          <div class="add-group">
            <button type="button" class="add" @click=${()=>this._add("entity")}>+ 状态徽章</button>
            <button type="button" class="add" @click=${()=>this._add("button")}>+ 控制徽章</button>
          </div>
        </div>
        ${this.badges.length===0?u`<p class="empty">可添加状态徽章（只读）或控制徽章（点击写值）</p>`:this.badges.map((e,t)=>{const i=e.type==="button"?"button":"entity";return u`
                <div class="badge-row">
                  <label class="field">
                    <span>类型</span>
                    <select
                      @change=${r=>this._changeType(t,r.target.value)}
                    >
                      <option value="entity" ?selected=${i==="entity"}>状态徽章</option>
                      <option value="button" ?selected=${i==="button"}>控制徽章</option>
                    </select>
                  </label>
                  <ha-entity-picker
                    label="动作"
                    .value=${String(e.entity??"")}
                    ?optional=${i==="button"}
                    @value-changed=${r=>this._patch(t,{entity:r.detail.value})}
                  ></ha-entity-picker>
                  <ha-icon-picker
                    label="图标"
                    .value=${String(e.icon??"")}
                    placeholder=${i==="button"?"mdi:gesture-tap-button":"mdi:thermometer"}
                    @icon-changed=${r=>this._patch(t,{icon:r.detail.value})}
                  ></ha-icon-picker>
                  <ha-color-picker
                    label="颜色"
                    .value=${String(e.color??"")}
                    ?includeState=${i==="entity"}
                    @color-changed=${r=>this._patch(t,{color:r.detail.value})}
                  ></ha-color-picker>
                  ${i==="entity"?u`
                        <label class="switch-row">
                          <span>显示图标</span>
                          <input
                            type="checkbox"
                            .checked=${e.show_icon!==!1}
                            @change=${r=>this._patch(t,{show_icon:r.target.checked})}
                          />
                        </label>
                        <label class="switch-row">
                          <span>显示文字</span>
                          <input
                            type="checkbox"
                            .checked=${Yt(e)}
                            @change=${r=>this._patch(t,{show_name:r.target.checked,show_state:r.target.checked})}
                          />
                        </label>
                      `:u`
                        <label class="field">
                          <span>文字</span>
                          <input
                            .value=${String(e.text??"")}
                            placeholder="触发"
                            @input=${r=>this._patch(t,{text:r.target.value})}
                          />
                        </label>
                        <label class="switch-row">
                          <span>显示文字</span>
                          <input
                            type="checkbox"
                            .checked=${Yt(e)}
                            @change=${r=>this._patch(t,{show_name:r.target.checked})}
                          />
                        </label>
                        <label class="field">
                          <span>写入值</span>
                          <input
                            .value=${e.value===void 0?"true":String(e.value)}
                            placeholder="true"
                            @input=${r=>this._patch(t,{value:rs(r.target.value)})}
                          />
                          <p class="helper">点击时向地址写入该值，默认 true</p>
                        </label>
                      `}
                  <button type="button" class="remove" @click=${()=>this._remove(t)}>
                    删除
                  </button>
                </div>
              `})}
      </div>
    `}};Xn.styles=$`
    .badges-editor {
      margin-bottom: 12px;
    }
    .head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 8px;
      gap: 8px;
    }
    .label {
      font-size: 13px;
      font-weight: 600;
      color: var(--primary-text-color);
    }
    .add-group {
      display: flex;
      gap: 12px;
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
    .empty {
      margin: 0 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
    }
    .badge-row {
      border: 1px solid var(--divider-color);
      border-radius: 10px;
      padding: 12px;
      margin-bottom: 8px;
      background: var(--secondary-background-color, #f8f9fa);
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 8px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field input,
    .field select {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      background: #fff;
      color: var(--primary-text-color);
    }
    .helper {
      margin: 0;
      font-size: 12px;
      color: var(--secondary-text-color);
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 8px;
      font-size: 13px;
      color: var(--primary-text-color);
      cursor: pointer;
    }
    .remove {
      border: none;
      background: transparent;
      color: var(--error-color, #b91c1c);
      font-size: 13px;
      cursor: pointer;
      padding: 0;
    }
  `;Mh([p({attribute:!1})],Xn.prototype,"badges",2);Xn=Mh([w("flow-heading-badges-editor")],Xn);var zy=Object.defineProperty,Ny=Object.getOwnPropertyDescriptor,zh=(e,t,i,r)=>{for(var o=r>1?void 0:r?Ny(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&zy(t,i,o),o};let Yn=class extends y{constructor(){super(...arguments),this.entities=[]}_emit(e){this.dispatchEvent(new CustomEvent("entities-changed",{bubbles:!0,composed:!0,detail:{entities:e}}))}_add(){this._emit([...this.entities,{entity:"",name:""}])}_remove(e){this._emit(this.entities.filter((t,i)=>i!==e))}_patch(e,t){this._emit(this.entities.map((i,r)=>r===e?{...i,...t}:i))}render(){return u`
      <div class="editor">
        <div class="head">
          <span class="label">柱（每柱绑定一个动作）</span>
          <button type="button" class="add" @click=${this._add}>+ 添加柱</button>
        </div>
        ${this.entities.length===0?u`<p class="empty">添加动作后，每根柱显示对应动作的实时数值</p>`:this.entities.map((e,t)=>u`
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
    `}};Yn.styles=$`
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
  `;zh([p({attribute:!1})],Yn.prototype,"entities",2);Yn=zh([w("flow-bar-entities-editor")],Yn);var Ry=Object.defineProperty,Ly=Object.getOwnPropertyDescriptor,Nh=(e,t,i,r)=>{for(var o=r>1?void 0:r?Ly(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ry(t,i,o),o};let Wn=class extends y{constructor(){super(...arguments),this.entities=[]}_emit(e){this.dispatchEvent(new CustomEvent("entities-changed",{bubbles:!0,composed:!0,detail:{entities:e}}))}_add(){this._emit([...this.entities,{entity:"",name:"",icon:""}])}_remove(e){this._emit(this.entities.filter((t,i)=>i!==e))}_patch(e,t){this._emit(this.entities.map((i,r)=>r===e?{...i,...t}:i))}render(){return u`
      <div class="editor">
        <div class="head">
          <span class="label">状态项（每项绑定一个动作）</span>
          <button type="button" class="add" @click=${this._add}>+ 添加状态</button>
        </div>
        ${this.entities.length===0?u`<p class="empty">添加动作后，每行显示对应动作的实时状态</p>`:this.entities.map((e,t)=>u`
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
    `}};Wn.styles=$`
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
  `;Nh([p({attribute:!1})],Wn.prototype,"entities",2);Wn=Nh([w("flow-status-entities-editor")],Wn);var By=Object.defineProperty,Fy=Object.getOwnPropertyDescriptor,ml=(e,t,i,r)=>{for(var o=r>1?void 0:r?Fy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&By(t,i,o),o};let Co=class extends y{constructor(){super(...arguments),this.elements=[],this._openIndex=0}get selectedIndex(){return this._openIndex!==null&&this.elements[this._openIndex]?this._openIndex:this.elements.length>0?0:-1}applyPosition(e,t){const i=this.selectedIndex;i<0||this._emit(this.elements.map((r,o)=>o===i?wn(r,t,e):r))}_emit(e){this.dispatchEvent(new CustomEvent("elements-changed",{bubbles:!0,composed:!0,detail:{elements:e}}))}_add(e){const t=e==="button"?{type:"button",entity:"",icon:"mdi:gesture-tap-button",text:"触发",value:!0,show_name:!0,style:{top:"50%",left:"50%"}}:{type:"state-badge",entity:"",icon:"mdi:thermometer",show_icon:!0,show_name:!0,style:{top:"40%",left:"40%"}},i=[...this.elements,t];this._openIndex=i.length-1,this._emit(i)}_remove(e){const t=this.elements.filter((i,r)=>r!==e);this._openIndex=t.length===0?null:Math.min(e,t.length-1),this._emit(t)}_duplicate(e){var n,s;const t=this.elements[e];if(!t)return;const i={top:Math.min(90,ot((n=t.style)==null?void 0:n.top,50)+6),left:Math.min(90,ot((s=t.style)==null?void 0:s.left,50)+6)},r=wn({...t},i.top,i.left),o=[...this.elements.slice(0,e+1),r,...this.elements.slice(e+1)];this._openIndex=e+1,this._emit(o)}_move(e,t){const i=e+t;if(i<0||i>=this.elements.length)return;const r=[...this.elements],[o]=r.splice(e,1);r.splice(i,0,o),this._openIndex=i,this._emit(r)}_patch(e,t){this._emit(this.elements.map((i,r)=>r===e?{...i,...t}:i))}_changeType(e,t){const i=this.elements[e];if(t==="button"){this._patch(e,{type:"button",text:String((i==null?void 0:i.text)??(i==null?void 0:i.name)??"触发"),value:(i==null?void 0:i.value)??!0,icon:(i==null?void 0:i.icon)||"mdi:gesture-tap-button",show_name:(i==null?void 0:i.show_name)??!0});return}this._patch(e,{type:"state-badge",icon:(i==null?void 0:i.icon)||"mdi:thermometer",show_icon:(i==null?void 0:i.show_icon)??!0,show_name:(i==null?void 0:i.show_name)??(i==null?void 0:i.show_state)??!0})}_setPos(e,t,i){var s,a;const r=this.elements[e];if(!r)return;const o=ot(t==="top"?i:(s=r.style)==null?void 0:s.top,50),n=ot(t==="left"?i:(a=r.style)==null?void 0:a.left,50);this._emit(this.elements.map((l,c)=>c===e?wn(l,o,n):l))}_title(e){return e.type==="button"?"控制徽章":"状态徽章"}_subtitle(e){return String(e.entity||e.text||e.name||"未选择地址")}render(){return u`
      <div class="elements-editor">
        <div class="head">
          <span class="label">元素</span>
          <div class="add-group">
            <button type="button" class="add" @click=${()=>this._add("state-badge")}>+ 状态徽章</button>
            <button type="button" class="add" @click=${()=>this._add("button")}>+ 控制徽章</button>
          </div>
        </div>
        <p class="hint">选中一项后，可在右侧预览图上点击或拖动徽章来定位。</p>
        ${this.elements.length===0?u`<p class="empty">还没有元素。添加后会叠在底图上。</p>`:this.elements.map((e,t)=>{var o,n;const i=e.type==="button"?"button":"state-badge",r=this._openIndex===t;return u`
                <div class="row ${r?"open":""}">
                  <div class="row-head">
                    <div class="meta" @click=${()=>{this._openIndex=r?null:t}}>
                      <strong>${this._title(e)}</strong>
                      <span>${this._subtitle(e)}</span>
                    </div>
                    <div class="actions">
                      <button type="button" title="上移" @click=${()=>this._move(t,-1)}>↑</button>
                      <button type="button" title="下移" @click=${()=>this._move(t,1)}>↓</button>
                      <button type="button" title="复制" @click=${()=>this._duplicate(t)}>
                        <ha-icon icon="mdi:content-copy"></ha-icon>
                      </button>
                      <button type="button" class="danger" title="删除" @click=${()=>this._remove(t)}>
                        <ha-icon icon="mdi:delete"></ha-icon>
                      </button>
                    </div>
                  </div>
                  ${r?u`
                        <div class="row-body">
                          <label class="field">
                            <span>类型</span>
                            <select
                              @change=${s=>this._changeType(t,s.target.value)}
                            >
                              <option value="state-badge" ?selected=${i==="state-badge"}>状态徽章</option>
                              <option value="button" ?selected=${i==="button"}>控制徽章</option>
                            </select>
                          </label>
                          <ha-entity-picker
                            label="动作"
                            .value=${String(e.entity??"")}
                            ?optional=${i==="button"}
                            @value-changed=${s=>this._patch(t,{entity:s.detail.value})}
                          ></ha-entity-picker>
                          <ha-icon-picker
                            label="图标"
                            .value=${String(e.icon??"")}
                            placeholder=${i==="button"?"mdi:gesture-tap-button":"mdi:thermometer"}
                            @icon-changed=${s=>this._patch(t,{icon:s.detail.value})}
                          ></ha-icon-picker>
                          <ha-color-picker
                            label="颜色"
                            .value=${String(e.color??"")}
                            ?includeState=${i==="state-badge"}
                            @color-changed=${s=>this._patch(t,{color:s.detail.value})}
                          ></ha-color-picker>
                          ${i==="state-badge"?u`
                                <label class="switch-row">
                                  <span>显示图标</span>
                                  <input
                                    type="checkbox"
                                    .checked=${e.show_icon!==!1}
                                    @change=${s=>this._patch(t,{show_icon:s.target.checked})}
                                  />
                                </label>
                                <label class="switch-row">
                                  <span>显示文字</span>
                                  <input
                                    type="checkbox"
                                    .checked=${Yt(e)}
                                    @change=${s=>this._patch(t,{show_name:s.target.checked})}
                                  />
                                </label>
                                <label class="field">
                                  <span>单位</span>
                                  <input
                                    .value=${String(e.unit??"")}
                                    placeholder="kWh"
                                    @input=${s=>this._patch(t,{unit:s.target.value})}
                                  />
                                </label>
                              `:u`
                                <label class="field">
                                  <span>文字</span>
                                  <input
                                    .value=${String(e.text??e.name??"")}
                                    placeholder="触发"
                                    @input=${s=>this._patch(t,{text:s.target.value})}
                                  />
                                </label>
                                <label class="switch-row">
                                  <span>显示文字</span>
                                  <input
                                    type="checkbox"
                                    .checked=${Yt(e)}
                                    @change=${s=>this._patch(t,{show_name:s.target.checked})}
                                  />
                                </label>
                                <label class="field">
                                  <span>写入值</span>
                                  <input
                                    .value=${e.value===void 0?"true":String(e.value)}
                                    placeholder="true"
                                    @input=${s=>this._patch(t,{value:rs(s.target.value)})}
                                  />
                                  <p class="helper">点击时向地址写入该值，默认 true</p>
                                </label>
                              `}
                          <div class="pos">
                            <label class="field">
                              <span>上 (%)</span>
                              <input
                                type="number"
                                min="0"
                                max="100"
                                step="0.5"
                                .value=${String(ot((o=e.style)==null?void 0:o.top,50))}
                                @input=${s=>this._setPos(t,"top",s.target.value)}
                              />
                            </label>
                            <label class="field">
                              <span>左 (%)</span>
                              <input
                                type="number"
                                min="0"
                                max="100"
                                step="0.5"
                                .value=${String(ot((n=e.style)==null?void 0:n.left,50))}
                                @input=${s=>this._setPos(t,"left",s.target.value)}
                              />
                            </label>
                          </div>
                        </div>
                      `:m}
                </div>
              `})}
      </div>
    `}};Co.styles=$`
    .elements-editor {
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
    .row {
      border: 1px solid var(--divider-color);
      border-radius: 10px;
      margin-bottom: 8px;
      background: var(--secondary-background-color, #f8f9fa);
      overflow: hidden;
    }
    .row.open {
      border-color: color-mix(in srgb, var(--primary-color) 45%, var(--divider-color));
    }
    .row-head {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 8px 10px;
    }
    .meta {
      min-width: 0;
      flex: 1;
      cursor: pointer;
    }
    .meta strong {
      display: block;
      font-size: 14px;
    }
    .meta span {
      display: block;
      font-size: 12px;
      color: var(--secondary-text-color);
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .actions {
      display: flex;
      align-items: center;
    }
    .actions button {
      border: none;
      background: transparent;
      color: var(--secondary-text-color);
      cursor: pointer;
      width: 28px;
      height: 28px;
      padding: 0;
      display: inline-flex;
      align-items: center;
      justify-content: center;
    }
    .actions .danger {
      color: var(--error-color, #b91c1c);
    }
    ha-icon {
      --mdc-icon-size: 16px;
    }
    .row-body {
      padding: 0 12px 12px;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 8px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .field input,
    .field select {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      background: #fff;
      color: var(--primary-text-color);
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 8px;
      font-size: 13px;
      color: var(--primary-text-color);
      cursor: pointer;
    }
    .pos {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 12px;
    }
  `;ml([p({attribute:!1})],Co.prototype,"elements",2);ml([b()],Co.prototype,"_openIndex",2);Co=ml([w("flow-picture-elements-editor")],Co);var Hy=Object.defineProperty,Uy=Object.getOwnPropertyDescriptor,Rh=(e,t,i,r)=>{for(var o=r>1?void 0:r?Uy(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Hy(t,i,o),o};let Kn=class extends y{constructor(){super(...arguments),this.config={type:"tile"}}_data(){return vy(this.config)}_isVisible(e,t){if(!e.visible)return!0;const i=t[e.visible.field];return e.visible.operator==="eq"?i===e.visible.value:i!==e.visible.value}_emit(e){const t="content_layout"in e,i={...this._data(),...e},r=my(i,{layoutChanged:t}),o={...this.config,...r};r.text_only||delete o.text_only,Number(r.list_columns)!==2&&delete o.list_columns,this.dispatchEvent(new CustomEvent("config-changed",{bubbles:!0,composed:!0,detail:{config:o}}))}_setField(e,t){e&&this._emit({[e]:t})}applyPictureElementPosition(e,t){const i=this.renderRoot.querySelector("flow-picture-elements-editor");return!i||i.selectedIndex<0?!1:(i.applyPosition(e,t),!0)}_renderField(e,t){var i,r,o,n,s;if(!this._isVisible(e,t))return m;switch(e.type){case"section":return u`
          <ha-expandable-panel .header=${e.label}>
            ${(i=e.schema)==null?void 0:i.map(a=>this._renderField(a,t))}
          </ha-expandable-panel>
        `;case"grid":{const a=e.gridVariant==="appearance";return u`
          <div class="grid ${a?"appearance":""}">
            ${(r=e.schema)==null?void 0:r.map(l=>this._renderField(l,t))}
          </div>
          ${a?u`<p class="appearance-helper">非活动状态（例如关闭或闭合）将不会被着色。</p>`:m}
        `}case"entity":return u`
          <ha-entity-picker
            .label=${e.label}
            .value=${String(t[e.name]??"")}
            .placeholder=${e.placeholder??""}
            ?optional=${e.optional}
            @value-changed=${a=>{const l={[e.name]:a.detail.value};a.detail.actionName&&e.name==="entity"&&(l.name=a.detail.actionName),this._emit(l)}}
          ></ha-entity-picker>
          ${e.helper?u`<p class="helper">${e.helper}</p>`:m}
        `;case"icon":return u`
          <ha-icon-picker
            .label=${e.label}
            .value=${String(t[e.name]??"")}
            .placeholder=${e.placeholder??"mdi:home"}
            ?compact=${e.compact}
            @icon-changed=${a=>this._setField(e.name,a.detail.value)}
          ></ha-icon-picker>
        `;case"color":return u`
          <ha-color-picker
            .label=${e.label}
            .value=${String(t[e.name]??"")}
            ?compact=${e.compact}
            ?hideHelper=${e.compact}
            @color-changed=${a=>this._setField(e.name,a.detail.value)}
          ></ha-color-picker>
        `;case"boolean":return u`
          <label class="switch-row">
            <div>
              <div class="switch-label">${e.label}</div>
              ${e.helper?u`<div class="helper">${e.helper}</div>`:m}
            </div>
            <input
              type="checkbox"
              .checked=${!!t[e.name]}
              @change=${a=>this._setField(e.name,a.target.checked)}
            />
          </label>
        `;case"select":return u`
          <label class="field">
            <span class="label-text">
              ${e.label}
              ${e.optional?u`<span class="optional">（可选）</span>`:m}
            </span>
            <select
              @change=${a=>this._setField(e.name,a.target.value)}
            >
              ${(o=e.options)==null?void 0:o.map(a=>{var c,d;const l=String(t[e.name]??((d=(c=e.options)==null?void 0:c[0])==null?void 0:d.value)??"");return u`
                  <option
                    value=${a.value}
                    ?selected=${l===a.value}
                    title=${a.description??""}
                  >
                    ${a.label}
                  </option>
                `})}
            </select>
            ${e.helper?u`<p class="helper">${e.helper}</p>`:m}
          </label>
        `;case"select_box":{const a=String(t[e.name]??((s=(n=e.options)==null?void 0:n[0])==null?void 0:s.value)??"");return u`
          <ha-select-box
            .label=${e.label}
            .value=${a}
            .options=${e.options??[]}
            .maxColumns=${e.maxColumns??2}
            @value-changed=${l=>{l.stopPropagation(),this._setField(e.name,l.detail.value)}}
          ></ha-select-box>
          ${e.helper?u`<p class="helper">${e.helper}</p>`:m}
        `}case"textarea":return u`
          <label class="field">
            <span class="label-text">${e.label}</span>
            <textarea
              rows="6"
              .value=${String(t[e.name]??"")}
              @input=${a=>this._setField(e.name,a.target.value)}
            ></textarea>
            ${e.helper?u`<p class="helper">${e.helper}</p>`:m}
          </label>
        `;case"number":return u`
          <label class="field">
            <span class="label-text">${e.label}</span>
            <input
              type="number"
              .value=${String(t[e.name]??"")}
              @input=${a=>this._setField(e.name,Number(a.target.value)||0)}
            />
          </label>
        `;case"badges":return u`
          <flow-heading-badges-editor
              .badges=${t.badges??[]}
            @badges-changed=${a=>{a.stopPropagation(),this._setField("badges",a.detail.badges)}}
          ></flow-heading-badges-editor>
        `;case"bar_entities":return u`
          <flow-bar-entities-editor
            .entities=${t.entities??[]}
            @entities-changed=${a=>{a.stopPropagation(),this._setField("entities",a.detail.entities)}}
          ></flow-bar-entities-editor>
        `;case"status_entities":return u`
          <flow-status-entities-editor
            .entities=${t.entities??[]}
            @entities-changed=${a=>{a.stopPropagation(),this._setField("entities",a.detail.entities)}}
          ></flow-status-entities-editor>
        `;case"image":return u`
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
                  @change=${a=>{var d;const l=(d=a.target.files)==null?void 0:d[0];if(a.target.value="",!l)return;const c=new FileReader;c.onload=()=>this._setField(e.name,String(c.result??"")),c.readAsDataURL(l)}}
                />
              </label>
              <button
                type="button"
                class="link-btn"
                @click=${()=>this._setField(e.name,Zd)}
              >
                使用默认户型图
              </button>
            </div>
            ${e.helper?u`<p class="helper">${e.helper}</p>`:m}
          </label>
        `;case"picture_elements":return u`
          <flow-picture-elements-editor
            .elements=${t.elements??[]}
            @elements-changed=${a=>{a.stopPropagation(),this._setField("elements",a.detail.elements)}}
          ></flow-picture-elements-editor>
        `;case"text":default:return u`
          <label class="field">
            <span class="label-text">
              ${e.label}
              ${e.optional?u`<span class="optional">（可选）</span>`:m}
            </span>
            <input
              type="text"
              .value=${String(t[e.name]??"")}
              placeholder=${e.placeholder??""}
              @input=${a=>this._setField(e.name,a.target.value)}
            />
            ${e.helper?u`<p class="helper">${e.helper}</p>`:m}
          </label>
        `}}render(){const e=String(this.config.type??"tile"),t=fy(e),i=this._data();return u`
      <div class="config-editor">
        <label class="field type-field">
          <span class="label-text">类型</span>
          <input type="text" .value=${e} disabled />
        </label>
        ${t.map(r=>this._renderField(r,i))}
      </div>
    `}};Kn.styles=$`
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
  `;Rh([p({attribute:!1})],Kn.prototype,"config",2);Kn=Rh([w("flow-card-config-editor")],Kn);/*! Hammer.JS - v2.0.17-rc - 2019-12-16
 * http://naver.github.io/egjs
 *
 * Forked By Naver egjs
 * Copyright (c) hammerjs
 * Licensed under the MIT license */function Qe(){return Qe=Object.assign||function(e){for(var t=1;t<arguments.length;t++){var i=arguments[t];for(var r in i)Object.prototype.hasOwnProperty.call(i,r)&&(e[r]=i[r])}return e},Qe.apply(this,arguments)}function Ve(e,t){e.prototype=Object.create(t.prototype),e.prototype.constructor=e,e.__proto__=t}function un(e){if(e===void 0)throw new ReferenceError("this hasn't been initialised - super() hasn't been called");return e}var fa;typeof Object.assign!="function"?fa=function(t){if(t==null)throw new TypeError("Cannot convert undefined or null to object");for(var i=Object(t),r=1;r<arguments.length;r++){var o=arguments[r];if(o!=null)for(var n in o)o.hasOwnProperty(n)&&(i[n]=o[n])}return i}:fa=Object.assign;var Oi=fa,Lc=["","webkit","Moz","MS","ms","o"],jy=typeof document>"u"?{style:{}}:document.createElement("div"),Vy="function",Qi=Math.round,Ai=Math.abs,vl=Date.now;function bs(e,t){for(var i,r,o=t[0].toUpperCase()+t.slice(1),n=0;n<Lc.length;){if(i=Lc[n],r=i?i+o:t,r in e)return r;n++}}var kt;typeof window>"u"?kt={}:kt=window;var Lh=bs(jy.style,"touchAction"),Bh=Lh!==void 0;function qy(){if(!Bh)return!1;var e={},t=kt.CSS&&kt.CSS.supports;return["auto","manipulation","pan-y","pan-x","pan-x pan-y","none"].forEach(function(i){return e[i]=t?kt.CSS.supports("touch-action",i):!0}),e}var Fh="compute",Hh="auto",ga="manipulation",vi="none",Eo="pan-x",Po="pan-y",pn=qy(),Gy=/mobile|tablet|ip(ad|hone|od)|android/i,Uh="ontouchstart"in kt,Xy=bs(kt,"PointerEvent")!==void 0,Yy=Uh&&Gy.test(navigator.userAgent),qo="touch",Wy="pen",bl="mouse",Ky="kinect",Zy=25,ue=1,Fi=2,ie=4,ye=8,Zn=1,Go=2,Xo=4,Yo=8,kr=16,lt=Go|Xo,bi=Yo|kr,_l=lt|bi,jh=["x","y"],Qn=["clientX","clientY"];function St(e,t,i){var r;if(e)if(e.forEach)e.forEach(t,i);else if(e.length!==void 0)for(r=0;r<e.length;)t.call(i,e[r],r,e),r++;else for(r in e)e.hasOwnProperty(r)&&t.call(i,e[r],r,e)}function _s(e,t){return typeof e===Vy?e.apply(t&&t[0]||void 0,t):e}function fi(e,t){return e.indexOf(t)>-1}function Qy(e){if(fi(e,vi))return vi;var t=fi(e,Eo),i=fi(e,Po);return t&&i?vi:t||i?t?Eo:Po:fi(e,ga)?ga:Hh}var Vh=function(){function e(i,r){this.manager=i,this.set(r)}var t=e.prototype;return t.set=function(r){r===Fh&&(r=this.compute()),Bh&&this.manager.element.style&&pn[r]&&(this.manager.element.style[Lh]=r),this.actions=r.toLowerCase().trim()},t.update=function(){this.set(this.manager.options.touchAction)},t.compute=function(){var r=[];return St(this.manager.recognizers,function(o){_s(o.options.enable,[o])&&(r=r.concat(o.getTouchAction()))}),Qy(r.join(" "))},t.preventDefaults=function(r){var o=r.srcEvent,n=r.offsetDirection;if(this.manager.session.prevented){o.preventDefault();return}var s=this.actions,a=fi(s,vi)&&!pn[vi],l=fi(s,Po)&&!pn[Po],c=fi(s,Eo)&&!pn[Eo];if(a){var d=r.pointers.length===1,h=r.distance<2,f=r.deltaTime<250;if(d&&h&&f)return}if(!(c&&l)&&(a||l&&n&lt||c&&n&bi))return this.preventSrc(o)},t.preventSrc=function(r){this.manager.session.prevented=!0,r.preventDefault()},e}();function yl(e,t){for(;e;){if(e===t)return!0;e=e.parentNode}return!1}function qh(e){var t=e.length;if(t===1)return{x:Qi(e[0].clientX),y:Qi(e[0].clientY)};for(var i=0,r=0,o=0;o<t;)i+=e[o].clientX,r+=e[o].clientY,o++;return{x:Qi(i/t),y:Qi(r/t)}}function Bc(e){for(var t=[],i=0;i<e.pointers.length;)t[i]={clientX:Qi(e.pointers[i].clientX),clientY:Qi(e.pointers[i].clientY)},i++;return{timeStamp:vl(),pointers:t,center:qh(t),deltaX:e.deltaX,deltaY:e.deltaY}}function Jn(e,t,i){i||(i=jh);var r=t[i[0]]-e[i[0]],o=t[i[1]]-e[i[1]];return Math.sqrt(r*r+o*o)}function ma(e,t,i){i||(i=jh);var r=t[i[0]]-e[i[0]],o=t[i[1]]-e[i[1]];return Math.atan2(o,r)*180/Math.PI}function Gh(e,t){return e===t?Zn:Ai(e)>=Ai(t)?e<0?Go:Xo:t<0?Yo:kr}function Jy(e,t){var i=t.center,r=e.offsetDelta||{},o=e.prevDelta||{},n=e.prevInput||{};(t.eventType===ue||n.eventType===ie)&&(o=e.prevDelta={x:n.deltaX||0,y:n.deltaY||0},r=e.offsetDelta={x:i.x,y:i.y}),t.deltaX=o.x+(i.x-r.x),t.deltaY=o.y+(i.y-r.y)}function Xh(e,t,i){return{x:t/e||0,y:i/e||0}}function ex(e,t){return Jn(t[0],t[1],Qn)/Jn(e[0],e[1],Qn)}function tx(e,t){return ma(t[1],t[0],Qn)+ma(e[1],e[0],Qn)}function ix(e,t){var i=e.lastInterval||t,r=t.timeStamp-i.timeStamp,o,n,s,a;if(t.eventType!==ye&&(r>Zy||i.velocity===void 0)){var l=t.deltaX-i.deltaX,c=t.deltaY-i.deltaY,d=Xh(r,l,c);n=d.x,s=d.y,o=Ai(d.x)>Ai(d.y)?d.x:d.y,a=Gh(l,c),e.lastInterval=t}else o=i.velocity,n=i.velocityX,s=i.velocityY,a=i.direction;t.velocity=o,t.velocityX=n,t.velocityY=s,t.direction=a}function rx(e,t){var i=e.session,r=t.pointers,o=r.length;i.firstInput||(i.firstInput=Bc(t)),o>1&&!i.firstMultiple?i.firstMultiple=Bc(t):o===1&&(i.firstMultiple=!1);var n=i.firstInput,s=i.firstMultiple,a=s?s.center:n.center,l=t.center=qh(r);t.timeStamp=vl(),t.deltaTime=t.timeStamp-n.timeStamp,t.angle=ma(a,l),t.distance=Jn(a,l),Jy(i,t),t.offsetDirection=Gh(t.deltaX,t.deltaY);var c=Xh(t.deltaTime,t.deltaX,t.deltaY);t.overallVelocityX=c.x,t.overallVelocityY=c.y,t.overallVelocity=Ai(c.x)>Ai(c.y)?c.x:c.y,t.scale=s?ex(s.pointers,r):1,t.rotation=s?tx(s.pointers,r):0,t.maxPointers=i.prevInput?t.pointers.length>i.prevInput.maxPointers?t.pointers.length:i.prevInput.maxPointers:t.pointers.length,ix(i,t);var d=e.element,h=t.srcEvent,f;h.composedPath?f=h.composedPath()[0]:h.path?f=h.path[0]:f=h.target,yl(f,d)&&(d=f),t.target=d}function ox(e,t,i){var r=i.pointers.length,o=i.changedPointers.length,n=t&ue&&r-o===0,s=t&(ie|ye)&&r-o===0;i.isFirst=!!n,i.isFinal=!!s,n&&(e.session={}),i.eventType=t,rx(e,i),e.emit("hammer.input",i),e.recognize(i),e.session.prevInput=i}function To(e){return e.trim().split(/\s+/g)}function ho(e,t,i){St(To(t),function(r){e.addEventListener(r,i,!1)})}function uo(e,t,i){St(To(t),function(r){e.removeEventListener(r,i,!1)})}function Fc(e){var t=e.ownerDocument||e;return t.defaultView||t.parentWindow||window}var Rr=function(){function e(i,r){var o=this;this.manager=i,this.callback=r,this.element=i.element,this.target=i.options.inputTarget,this.domHandler=function(n){_s(i.options.enable,[i])&&o.handler(n)},this.init()}var t=e.prototype;return t.handler=function(){},t.init=function(){this.evEl&&ho(this.element,this.evEl,this.domHandler),this.evTarget&&ho(this.target,this.evTarget,this.domHandler),this.evWin&&ho(Fc(this.element),this.evWin,this.domHandler)},t.destroy=function(){this.evEl&&uo(this.element,this.evEl,this.domHandler),this.evTarget&&uo(this.target,this.evTarget,this.domHandler),this.evWin&&uo(Fc(this.element),this.evWin,this.domHandler)},e}();function Di(e,t,i){if(e.indexOf&&!i)return e.indexOf(t);for(var r=0;r<e.length;){if(i&&e[r][i]==t||!i&&e[r]===t)return r;r++}return-1}var nx={pointerdown:ue,pointermove:Fi,pointerup:ie,pointercancel:ye,pointerout:ye},sx={2:qo,3:Wy,4:bl,5:Ky},Yh="pointerdown",Wh="pointermove pointerup pointercancel";kt.MSPointerEvent&&!kt.PointerEvent&&(Yh="MSPointerDown",Wh="MSPointerMove MSPointerUp MSPointerCancel");var Kh=function(e){Ve(t,e);function t(){var r,o=t.prototype;return o.evEl=Yh,o.evWin=Wh,r=e.apply(this,arguments)||this,r.store=r.manager.session.pointerEvents=[],r}var i=t.prototype;return i.handler=function(o){var n=this.store,s=!1,a=o.type.toLowerCase().replace("ms",""),l=nx[a],c=sx[o.pointerType]||o.pointerType,d=c===qo,h=Di(n,o.pointerId,"pointerId");l&ue&&(o.button===0||d)?h<0&&(n.push(o),h=n.length-1):l&(ie|ye)&&(s=!0),!(h<0)&&(n[h]=o,this.callback(this.manager,l,{pointers:n,changedPointers:[o],pointerType:c,srcEvent:o}),s&&n.splice(h,1))},t}(Rr);function Oo(e){return Array.prototype.slice.call(e,0)}function xl(e,t,i){for(var r=[],o=[],n=0;n<e.length;){var s=t?e[n][t]:e[n];Di(o,s)<0&&r.push(e[n]),o[n]=s,n++}return i&&(t?r=r.sort(function(a,l){return a[t]>l[t]}):r=r.sort()),r}var ax={touchstart:ue,touchmove:Fi,touchend:ie,touchcancel:ye},lx="touchstart touchmove touchend touchcancel",wl=function(e){Ve(t,e);function t(){var r;return t.prototype.evTarget=lx,r=e.apply(this,arguments)||this,r.targetIds={},r}var i=t.prototype;return i.handler=function(o){var n=ax[o.type],s=cx.call(this,o,n);s&&this.callback(this.manager,n,{pointers:s[0],changedPointers:s[1],pointerType:qo,srcEvent:o})},t}(Rr);function cx(e,t){var i=Oo(e.touches),r=this.targetIds;if(t&(ue|Fi)&&i.length===1)return r[i[0].identifier]=!0,[i,i];var o,n,s=Oo(e.changedTouches),a=[],l=this.target;if(n=i.filter(function(c){return yl(c.target,l)}),t===ue)for(o=0;o<n.length;)r[n[o].identifier]=!0,o++;for(o=0;o<s.length;)r[s[o].identifier]&&a.push(s[o]),t&(ie|ye)&&delete r[s[o].identifier],o++;if(a.length)return[xl(n.concat(a),"identifier",!0),a]}var dx={mousedown:ue,mousemove:Fi,mouseup:ie},hx="mousedown",ux="mousemove mouseup",$l=function(e){Ve(t,e);function t(){var r,o=t.prototype;return o.evEl=hx,o.evWin=ux,r=e.apply(this,arguments)||this,r.pressed=!1,r}var i=t.prototype;return i.handler=function(o){var n=dx[o.type];n&ue&&o.button===0&&(this.pressed=!0),n&Fi&&o.which!==1&&(n=ie),this.pressed&&(n&ie&&(this.pressed=!1),this.callback(this.manager,n,{pointers:[o],changedPointers:[o],pointerType:bl,srcEvent:o}))},t}(Rr),px=2500,Hc=25;function Uc(e){var t=e.changedPointers,i=t[0];if(i.identifier===this.primaryTouch){var r={x:i.clientX,y:i.clientY},o=this.lastTouches;this.lastTouches.push(r);var n=function(){var a=o.indexOf(r);a>-1&&o.splice(a,1)};setTimeout(n,px)}}function fx(e,t){e&ue?(this.primaryTouch=t.changedPointers[0].identifier,Uc.call(this,t)):e&(ie|ye)&&Uc.call(this,t)}function gx(e){for(var t=e.srcEvent.clientX,i=e.srcEvent.clientY,r=0;r<this.lastTouches.length;r++){var o=this.lastTouches[r],n=Math.abs(t-o.x),s=Math.abs(i-o.y);if(n<=Hc&&s<=Hc)return!0}return!1}var Zh=function(){var e=function(t){Ve(i,t);function i(o,n){var s;return s=t.call(this,o,n)||this,s.handler=function(a,l,c){var d=c.pointerType===qo,h=c.pointerType===bl;if(!(h&&c.sourceCapabilities&&c.sourceCapabilities.firesTouchEvents)){if(d)fx.call(un(un(s)),l,c);else if(h&&gx.call(un(un(s)),c))return;s.callback(a,l,c)}},s.touch=new wl(s.manager,s.handler),s.mouse=new $l(s.manager,s.handler),s.primaryTouch=null,s.lastTouches=[],s}var r=i.prototype;return r.destroy=function(){this.touch.destroy(),this.mouse.destroy()},i}(Rr);return e}();function mx(e){var t,i=e.options.inputClass;return i?t=i:Xy?t=Kh:Yy?t=wl:Uh?t=Zh:t=$l,new t(e,ox)}function Ji(e,t,i){return Array.isArray(e)?(St(e,i[t],i),!0):!1}var En=1,Ne=2,Sr=4,Gt=8,Ct=Gt,Ao=16,st=32,vx=1;function bx(){return vx++}function fn(e,t){var i=t.manager;return i?i.get(e):e}function jc(e){return e&Ao?"cancel":e&Gt?"end":e&Sr?"move":e&Ne?"start":""}var Wo=function(){function e(i){i===void 0&&(i={}),this.options=Qe({enable:!0},i),this.id=bx(),this.manager=null,this.state=En,this.simultaneous={},this.requireFail=[]}var t=e.prototype;return t.set=function(r){return Oi(this.options,r),this.manager&&this.manager.touchAction.update(),this},t.recognizeWith=function(r){if(Ji(r,"recognizeWith",this))return this;var o=this.simultaneous;return r=fn(r,this),o[r.id]||(o[r.id]=r,r.recognizeWith(this)),this},t.dropRecognizeWith=function(r){return Ji(r,"dropRecognizeWith",this)?this:(r=fn(r,this),delete this.simultaneous[r.id],this)},t.requireFailure=function(r){if(Ji(r,"requireFailure",this))return this;var o=this.requireFail;return r=fn(r,this),Di(o,r)===-1&&(o.push(r),r.requireFailure(this)),this},t.dropRequireFailure=function(r){if(Ji(r,"dropRequireFailure",this))return this;r=fn(r,this);var o=Di(this.requireFail,r);return o>-1&&this.requireFail.splice(o,1),this},t.hasRequireFailures=function(){return this.requireFail.length>0},t.canRecognizeWith=function(r){return!!this.simultaneous[r.id]},t.emit=function(r){var o=this,n=this.state;function s(a){o.manager.emit(a,r)}n<Gt&&s(o.options.event+jc(n)),s(o.options.event),r.additionalEvent&&s(r.additionalEvent),n>=Gt&&s(o.options.event+jc(n))},t.tryEmit=function(r){if(this.canEmit())return this.emit(r);this.state=st},t.canEmit=function(){for(var r=0;r<this.requireFail.length;){if(!(this.requireFail[r].state&(st|En)))return!1;r++}return!0},t.recognize=function(r){var o=Oi({},r);if(!_s(this.options.enable,[this,o])){this.reset(),this.state=st;return}this.state&(Ct|Ao|st)&&(this.state=En),this.state=this.process(o),this.state&(Ne|Sr|Gt|Ao)&&this.tryEmit(o)},t.process=function(r){},t.getTouchAction=function(){},t.reset=function(){},e}(),es=function(e){Ve(t,e);function t(r){var o;return r===void 0&&(r={}),o=e.call(this,Qe({event:"tap",pointers:1,taps:1,interval:300,time:250,threshold:9,posThreshold:10},r))||this,o.pTime=!1,o.pCenter=!1,o._timer=null,o._input=null,o.count=0,o}var i=t.prototype;return i.getTouchAction=function(){return[ga]},i.process=function(o){var n=this,s=this.options,a=o.pointers.length===s.pointers,l=o.distance<s.threshold,c=o.deltaTime<s.time;if(this.reset(),o.eventType&ue&&this.count===0)return this.failTimeout();if(l&&c&&a){if(o.eventType!==ie)return this.failTimeout();var d=this.pTime?o.timeStamp-this.pTime<s.interval:!0,h=!this.pCenter||Jn(this.pCenter,o.center)<s.posThreshold;this.pTime=o.timeStamp,this.pCenter=o.center,!h||!d?this.count=1:this.count+=1,this._input=o;var f=this.count%s.taps;if(f===0)return this.hasRequireFailures()?(this._timer=setTimeout(function(){n.state=Ct,n.tryEmit()},s.interval),Ne):Ct}return st},i.failTimeout=function(){var o=this;return this._timer=setTimeout(function(){o.state=st},this.options.interval),st},i.reset=function(){clearTimeout(this._timer)},i.emit=function(){this.state===Ct&&(this._input.tapCount=this.count,this.manager.emit(this.options.event,this._input))},t}(Wo),Cr=function(e){Ve(t,e);function t(r){return r===void 0&&(r={}),e.call(this,Qe({pointers:1},r))||this}var i=t.prototype;return i.attrTest=function(o){var n=this.options.pointers;return n===0||o.pointers.length===n},i.process=function(o){var n=this.state,s=o.eventType,a=n&(Ne|Sr),l=this.attrTest(o);return a&&(s&ye||!l)?n|Ao:a||l?s&ie?n|Gt:n&Ne?n|Sr:Ne:st},t}(Wo);function Qh(e){return e===kr?"down":e===Yo?"up":e===Go?"left":e===Xo?"right":""}var ys=function(e){Ve(t,e);function t(r){var o;return r===void 0&&(r={}),o=e.call(this,Qe({event:"pan",threshold:10,pointers:1,direction:_l},r))||this,o.pX=null,o.pY=null,o}var i=t.prototype;return i.getTouchAction=function(){var o=this.options.direction,n=[];return o&lt&&n.push(Po),o&bi&&n.push(Eo),n},i.directionTest=function(o){var n=this.options,s=!0,a=o.distance,l=o.direction,c=o.deltaX,d=o.deltaY;return l&n.direction||(n.direction&lt?(l=c===0?Zn:c<0?Go:Xo,s=c!==this.pX,a=Math.abs(o.deltaX)):(l=d===0?Zn:d<0?Yo:kr,s=d!==this.pY,a=Math.abs(o.deltaY))),o.direction=l,s&&a>n.threshold&&l&n.direction},i.attrTest=function(o){return Cr.prototype.attrTest.call(this,o)&&(this.state&Ne||!(this.state&Ne)&&this.directionTest(o))},i.emit=function(o){this.pX=o.deltaX,this.pY=o.deltaY;var n=Qh(o.direction);n&&(o.additionalEvent=this.options.event+n),e.prototype.emit.call(this,o)},t}(Cr),Jh=function(e){Ve(t,e);function t(r){return r===void 0&&(r={}),e.call(this,Qe({event:"swipe",threshold:10,velocity:.3,direction:lt|bi,pointers:1},r))||this}var i=t.prototype;return i.getTouchAction=function(){return ys.prototype.getTouchAction.call(this)},i.attrTest=function(o){var n=this.options.direction,s;return n&(lt|bi)?s=o.overallVelocity:n&lt?s=o.overallVelocityX:n&bi&&(s=o.overallVelocityY),e.prototype.attrTest.call(this,o)&&n&o.offsetDirection&&o.distance>this.options.threshold&&o.maxPointers===this.options.pointers&&Ai(s)>this.options.velocity&&o.eventType&ie},i.emit=function(o){var n=Qh(o.offsetDirection);n&&this.manager.emit(this.options.event+n,o),this.manager.emit(this.options.event,o)},t}(Cr),eu=function(e){Ve(t,e);function t(r){return r===void 0&&(r={}),e.call(this,Qe({event:"pinch",threshold:0,pointers:2},r))||this}var i=t.prototype;return i.getTouchAction=function(){return[vi]},i.attrTest=function(o){return e.prototype.attrTest.call(this,o)&&(Math.abs(o.scale-1)>this.options.threshold||this.state&Ne)},i.emit=function(o){if(o.scale!==1){var n=o.scale<1?"in":"out";o.additionalEvent=this.options.event+n}e.prototype.emit.call(this,o)},t}(Cr),tu=function(e){Ve(t,e);function t(r){return r===void 0&&(r={}),e.call(this,Qe({event:"rotate",threshold:0,pointers:2},r))||this}var i=t.prototype;return i.getTouchAction=function(){return[vi]},i.attrTest=function(o){return e.prototype.attrTest.call(this,o)&&(Math.abs(o.rotation)>this.options.threshold||this.state&Ne)},t}(Cr),iu=function(e){Ve(t,e);function t(r){var o;return r===void 0&&(r={}),o=e.call(this,Qe({event:"press",pointers:1,time:251,threshold:9},r))||this,o._timer=null,o._input=null,o}var i=t.prototype;return i.getTouchAction=function(){return[Hh]},i.process=function(o){var n=this,s=this.options,a=o.pointers.length===s.pointers,l=o.distance<s.threshold,c=o.deltaTime>s.time;if(this._input=o,!l||!a||o.eventType&(ie|ye)&&!c)this.reset();else if(o.eventType&ue)this.reset(),this._timer=setTimeout(function(){n.state=Ct,n.tryEmit()},s.time);else if(o.eventType&ie)return Ct;return st},i.reset=function(){clearTimeout(this._timer)},i.emit=function(o){this.state===Ct&&(o&&o.eventType&ie?this.manager.emit(this.options.event+"up",o):(this._input.timeStamp=vl(),this.manager.emit(this.options.event,this._input)))},t}(Wo),ru={domEvents:!1,touchAction:Fh,enable:!0,inputTarget:null,inputClass:null,cssProps:{userSelect:"none",touchSelect:"none",touchCallout:"none",contentZooming:"none",userDrag:"none",tapHighlightColor:"rgba(0,0,0,0)"}},Vc=[[tu,{enable:!1}],[eu,{enable:!1},["rotate"]],[Jh,{direction:lt}],[ys,{direction:lt},["swipe"]],[es],[es,{event:"doubletap",taps:2},["tap"]],[iu]],_x=1,qc=2;function Gc(e,t){var i=e.element;if(i.style){var r;St(e.options.cssProps,function(o,n){r=bs(i.style,n),t?(e.oldCssProps[r]=i.style[r],i.style[r]=o):i.style[r]=e.oldCssProps[r]||""}),t||(e.oldCssProps={})}}function yx(e,t){var i=document.createEvent("Event");i.initEvent(e,!0,!0),i.gesture=t,t.target.dispatchEvent(i)}var va=function(){function e(i,r){var o=this;this.options=Oi({},ru,r||{}),this.options.inputTarget=this.options.inputTarget||i,this.handlers={},this.session={},this.recognizers=[],this.oldCssProps={},this.element=i,this.input=mx(this),this.touchAction=new Vh(this,this.options.touchAction),Gc(this,!0),St(this.options.recognizers,function(n){var s=o.add(new n[0](n[1]));n[2]&&s.recognizeWith(n[2]),n[3]&&s.requireFailure(n[3])},this)}var t=e.prototype;return t.set=function(r){return Oi(this.options,r),r.touchAction&&this.touchAction.update(),r.inputTarget&&(this.input.destroy(),this.input.target=r.inputTarget,this.input.init()),this},t.stop=function(r){this.session.stopped=r?qc:_x},t.recognize=function(r){var o=this.session;if(!o.stopped){this.touchAction.preventDefaults(r);var n,s=this.recognizers,a=o.curRecognizer;(!a||a&&a.state&Ct)&&(o.curRecognizer=null,a=null);for(var l=0;l<s.length;)n=s[l],o.stopped!==qc&&(!a||n===a||n.canRecognizeWith(a))?n.recognize(r):n.reset(),!a&&n.state&(Ne|Sr|Gt)&&(o.curRecognizer=n,a=n),l++}},t.get=function(r){if(r instanceof Wo)return r;for(var o=this.recognizers,n=0;n<o.length;n++)if(o[n].options.event===r)return o[n];return null},t.add=function(r){if(Ji(r,"add",this))return this;var o=this.get(r.options.event);return o&&this.remove(o),this.recognizers.push(r),r.manager=this,this.touchAction.update(),r},t.remove=function(r){if(Ji(r,"remove",this))return this;var o=this.get(r);if(r){var n=this.recognizers,s=Di(n,o);s!==-1&&(n.splice(s,1),this.touchAction.update())}return this},t.on=function(r,o){if(r===void 0||o===void 0)return this;var n=this.handlers;return St(To(r),function(s){n[s]=n[s]||[],n[s].push(o)}),this},t.off=function(r,o){if(r===void 0)return this;var n=this.handlers;return St(To(r),function(s){o?n[s]&&n[s].splice(Di(n[s],o),1):delete n[s]}),this},t.emit=function(r,o){this.options.domEvents&&yx(r,o);var n=this.handlers[r]&&this.handlers[r].slice();if(!(!n||!n.length)){o.type=r,o.preventDefault=function(){o.srcEvent.preventDefault()};for(var s=0;s<n.length;)n[s](o),s++}},t.destroy=function(){this.element&&Gc(this,!1),this.handlers={},this.session={},this.input.destroy(),this.element=null},e}(),xx={touchstart:ue,touchmove:Fi,touchend:ie,touchcancel:ye},wx="touchstart",$x="touchstart touchmove touchend touchcancel",kx=function(e){Ve(t,e);function t(){var r,o=t.prototype;return o.evTarget=wx,o.evWin=$x,r=e.apply(this,arguments)||this,r.started=!1,r}var i=t.prototype;return i.handler=function(o){var n=xx[o.type];if(n===ue&&(this.started=!0),!!this.started){var s=Sx.call(this,o,n);n&(ie|ye)&&s[0].length-s[1].length===0&&(this.started=!1),this.callback(this.manager,n,{pointers:s[0],changedPointers:s[1],pointerType:qo,srcEvent:o})}},t}(Rr);function Sx(e,t){var i=Oo(e.touches),r=Oo(e.changedTouches);return t&(ie|ye)&&(i=xl(i.concat(r),"identifier",!0)),[i,r]}function ou(e,t,i){var r="DEPRECATED METHOD: "+t+`
`+i+` AT 
`;return function(){var o=new Error("get-stack-trace"),n=o&&o.stack?o.stack.replace(/^[^\(]+?[\n$]/gm,"").replace(/^\s+at\s+/gm,"").replace(/^Object.<anonymous>\s*\(/gm,"{anonymous}()@"):"Unknown Stack Trace",s=window.console&&(window.console.warn||window.console.log);return s&&s.call(window.console,r,n),e.apply(this,arguments)}}var nu=ou(function(e,t,i){for(var r=Object.keys(t),o=0;o<r.length;)(!i||i&&e[r[o]]===void 0)&&(e[r[o]]=t[r[o]]),o++;return e},"extend","Use `assign`."),Cx=ou(function(e,t){return nu(e,t,!0)},"merge","Use `assign`.");function Ex(e,t,i){var r=t.prototype,o;o=e.prototype=Object.create(r),o.constructor=e,o._super=r,i&&Oi(o,i)}function Xc(e,t){return function(){return e.apply(t,arguments)}}var Px=function(){var e=function(i,r){return r===void 0&&(r={}),new va(i,Qe({recognizers:Vc.concat()},r))};return e.VERSION="2.0.17-rc",e.DIRECTION_ALL=_l,e.DIRECTION_DOWN=kr,e.DIRECTION_LEFT=Go,e.DIRECTION_RIGHT=Xo,e.DIRECTION_UP=Yo,e.DIRECTION_HORIZONTAL=lt,e.DIRECTION_VERTICAL=bi,e.DIRECTION_NONE=Zn,e.DIRECTION_DOWN=kr,e.INPUT_START=ue,e.INPUT_MOVE=Fi,e.INPUT_END=ie,e.INPUT_CANCEL=ye,e.STATE_POSSIBLE=En,e.STATE_BEGAN=Ne,e.STATE_CHANGED=Sr,e.STATE_ENDED=Gt,e.STATE_RECOGNIZED=Ct,e.STATE_CANCELLED=Ao,e.STATE_FAILED=st,e.Manager=va,e.Input=Rr,e.TouchAction=Vh,e.TouchInput=wl,e.MouseInput=$l,e.PointerEventInput=Kh,e.TouchMouseInput=Zh,e.SingleTouchInput=kx,e.Recognizer=Wo,e.AttrRecognizer=Cr,e.Tap=es,e.Pan=ys,e.Swipe=Jh,e.Pinch=eu,e.Rotate=tu,e.Press=iu,e.on=ho,e.off=uo,e.each=St,e.merge=Cx,e.extend=nu,e.bindFn=Xc,e.assign=Oi,e.inherit=Ex,e.bindFn=Xc,e.prefixed=bs,e.toArray=Oo,e.inArray=Di,e.uniqueArray=xl,e.splitStr=To,e.boolOrFn=_s,e.hasParent=yl,e.addEventListeners=ho,e.removeEventListeners=uo,e.defaults=Oi({},ru,{preset:Vc}),e}();Px.defaults;var Tx=Object.defineProperty,Ox=Object.getOwnPropertyDescriptor,Ae=(e,t,i,r)=>{for(var o=r>1?void 0:r?Ox(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Tx(t,i,o),o};let pe=class extends y{constructor(){super(...arguments),this.disabled=!1,this.vertical=!1,this.tooltipMode="interaction",this.step=1,this.min=1,this.max=4,this.pressed=!1,this.tooltipVisible=!1,this._handleKeyDown=e=>{if(new Set(["ArrowRight","ArrowUp","ArrowLeft","ArrowDown","PageUp","PageDown","Home","End"]).has(e.code)){switch(e.preventDefault(),e.code){case"ArrowRight":case"ArrowUp":this.value=this._boundedValue((this.value??0)+this.step);break;case"ArrowLeft":case"ArrowDown":this.value=this._boundedValue((this.value??0)-this.step);break;case"PageUp":this.value=this._steppedValue(this._boundedValue((this.value??0)+this._tenPercentStep));break;case"PageDown":this.value=this._steppedValue(this._boundedValue((this.value??0)-this._tenPercentStep));break;case"Home":this.value=this.min;break;case"End":this.value=this.max;break}this.dispatchEvent(new CustomEvent("slider-moved",{bubbles:!0,composed:!0,detail:{value:this.value}}))}},this._handleKeyUp=e=>{new Set(["ArrowRight","ArrowUp","ArrowLeft","ArrowDown","PageUp","PageDown","Home","End"]).has(e.code)&&(e.preventDefault(),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:this.value}})))},this._getPercentageFromEvent=e=>{if(this.vertical){const o=e.center.y,n=e.target.getBoundingClientRect().top,s=e.target.clientHeight;return Math.max(Math.min(1,(o-n)/s),0)}const t=e.center.x,i=e.target.getBoundingClientRect().left,r=e.target.clientWidth;return Math.max(Math.min(1,(t-i)/r),0)}}get _range(){return this.range??this.max}_valueToPercentage(e){return this._boundedValue(e)/this._range}_percentageToValue(e){return this._range*e}_steppedValue(e){return Math.round(e/this.step)*this.step}_boundedValue(e){return Math.min(Math.max(e,this.min),this.max)}firstUpdated(e){super.firstUpdated(e),this.setupListeners(),this.setAttribute("role","slider"),this.hasAttribute("tabindex")||this.setAttribute("tabindex","0")}updated(e){if(super.updated(e),e.has("value")){const t=this._steppedValue(this.value??0);this.setAttribute("aria-valuenow",t.toString()),this.setAttribute("aria-valuetext",t.toString())}e.has("min")&&this.setAttribute("aria-valuemin",this.min.toString()),e.has("max")&&this.setAttribute("aria-valuemax",this.max.toString()),e.has("vertical")&&this.setAttribute("aria-orientation",this.vertical?"vertical":"horizontal")}connectedCallback(){super.connectedCallback(),this.setupListeners()}disconnectedCallback(){super.disconnectedCallback(),this.destroyListeners()}setupListeners(){if(this.slider&&!this._mc){this._mc=new va(this.slider,{touchAction:this.touchAction??(this.vertical?"pan-x":"pan-y")}),this._mc.add(new ys({threshold:10,direction:_l,enable:!0})),this._mc.add(new es({event:"singletap"}));let e;this._mc.on("panstart",()=>{this.disabled||(this.pressed=!0,this._showTooltip(),e=this.value)}),this._mc.on("pancancel",()=>{this.disabled||(this.pressed=!1,this._hideTooltip(),this.value=e)}),this._mc.on("panmove",t=>{if(this.disabled)return;const i=this._getPercentageFromEvent(t);this.value=this._percentageToValue(i);const r=this._steppedValue(this._boundedValue(this.value));this.dispatchEvent(new CustomEvent("slider-moved",{bubbles:!0,composed:!0,detail:{value:r}}))}),this._mc.on("panend",t=>{if(this.disabled)return;this.pressed=!1,this._hideTooltip();const i=this._getPercentageFromEvent(t),r=this._percentageToValue(i);this.value=this._steppedValue(this._boundedValue(r)),this.dispatchEvent(new CustomEvent("slider-moved",{bubbles:!0,composed:!0,detail:{value:void 0}})),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:this.value}}))}),this._mc.on("singletap",t=>{if(this.disabled)return;const i=this._getPercentageFromEvent(t),r=this._percentageToValue(i);this.value=this._steppedValue(this._boundedValue(r)),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:this.value}}))}),this.addEventListener("keydown",this._handleKeyDown),this.addEventListener("keyup",this._handleKeyUp)}}destroyListeners(){this._mc&&(this._mc.destroy(),this._mc=void 0),this.removeEventListener("keydown",this._handleKeyDown),this.removeEventListener("keyup",this._handleKeyUp)}get _tenPercentStep(){return Math.max(this.step,(this.max-this.min)/10)}_showTooltip(){this._tooltipTimeout!=null&&window.clearTimeout(this._tooltipTimeout),this.tooltipVisible=!0}_hideTooltip(e){if(!e){this.tooltipVisible=!1;return}this._tooltipTimeout=window.setTimeout(()=>{this.tooltipVisible=!1},e)}_renderTooltip(){if(this.tooltipMode==="never")return m;const e=this.vertical?"left":"top",t=this.tooltipMode==="always"||this.tooltipVisible&&this.tooltipMode==="interaction",i=this._boundedValue(this._steppedValue(this.value??0));return u`
      <div
        class="tooltip ${O({visible:t,[e]:!0})}"
      >
        ${i}
      </div>
    `}render(){return u`
      <div
        class="container ${O({pressed:this.pressed})}"
        style=${T({"--value":`${this._valueToPercentage(this.value??0)}`,"--min":`${this.min/this._range}`,"--max":`${1-this.max/this._range}`})}
      >
        <div id="slider" class="slider">
          <div class="track">
            <div class="background"></div>
            <div class="active"></div>
            ${Array(this._range/this.step).fill(0).map((e,t)=>this.min>=t*this.step||t*this.step>this.max?m:u`
                  <div
                    class="dot"
                    style=${T({"--value":`${t/(this._range/this.step)}`})}
                  ></div>
                `)}
            ${this.value!==void 0?u`<div class="handle"></div>`:m}
          </div>
          ${this._renderTooltip()}
        </div>
      </div>
    `}};pe.styles=$`
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
  `;Ae([p({type:Boolean,reflect:!0})],pe.prototype,"disabled",2);Ae([p({type:Boolean,reflect:!0})],pe.prototype,"vertical",2);Ae([p({attribute:"touch-action"})],pe.prototype,"touchAction",2);Ae([p({attribute:"tooltip-mode"})],pe.prototype,"tooltipMode",2);Ae([p({type:Number})],pe.prototype,"value",2);Ae([p({type:Number})],pe.prototype,"step",2);Ae([p({type:Number})],pe.prototype,"min",2);Ae([p({type:Number})],pe.prototype,"max",2);Ae([p({type:Number})],pe.prototype,"range",2);Ae([b()],pe.prototype,"pressed",2);Ae([b()],pe.prototype,"tooltipVisible",2);Ae([zo("#slider")],pe.prototype,"slider",2);pe=Ae([w("ha-grid-layout-slider")],pe);var Ax=Object.defineProperty,Dx=Object.getOwnPropertyDescriptor,xs=(e,t,i,r)=>{for(var o=r>1?void 0:r?Dx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ax(t,i,o),o};let Er=class extends y{constructor(){super(...arguments),this.path="",this.label="",this.disabled=!1}render(){const e=this.path||mi("mdi:help-circle-outline");return u`
      <button
        type="button"
        class="button"
        ?disabled=${this.disabled}
        aria-label=${this.label||m}
        title=${this.label||m}
      >
        <ha-svg-icon .path=${e}></ha-svg-icon>
      </button>
    `}};Er.styles=$`
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
  `;xs([p()],Er.prototype,"path",2);xs([p()],Er.prototype,"label",2);xs([p({type:Boolean})],Er.prototype,"disabled",2);Er=xs([w("ha-icon-button")],Er);var Ix=Object.defineProperty,Mx=Object.getOwnPropertyDescriptor,it=(e,t,i,r)=>{for(var o=r>1?void 0:r?Mx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Ix(t,i,o),o};function Yc(e,t,i){let r=e;return t!==void 0&&(r=Math.max(r,t)),i!==void 0&&(r=Math.min(r,i)),r}let Pe=class extends y{constructor(){super(...arguments),this.rows=8,this.columns=12,this.isDefault=!1,this.step=1,this._localValue={rows:1,columns:1}}willUpdate(e){e.has("value")&&(this._localValue=this.value)}_cellClick(e){const t=e.currentTarget,i=Number(t.getAttribute("data-row")),r=Number(t.getAttribute("data-column")),o=Yc(i,this.rowMin,this.rowMax);let n=Yc(r,this.columnMin,this.columnMax);(this.value??ir).columns==="full"&&n===this.columns&&(n="full"),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:{rows:o,columns:n}}}))}_valueChanged(e){e.stopPropagation();const t=e.currentTarget.id,i=this.value??ir;let r=e.detail.value;t==="columns"&&i.columns==="full"&&r===this.columns&&(r="full"),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:{...i,[t]:r}}}))}_reset(e){e.stopPropagation(),this.dispatchEvent(new CustomEvent("value-changed",{bubbles:!0,composed:!0,detail:{value:{rows:void 0,columns:void 0}}}))}_sliderMoved(e){e.stopPropagation();const t=e.currentTarget.id,i=this.value??ir,r=e.detail.value;r!==void 0&&(this._localValue={...i,[t]:r})}render(){var f,v,g,_,k,S;const e=((f=this._localValue)==null?void 0:f.rows)==="auto",t=((v=this._localValue)==null?void 0:v.columns)==="full",i=t||this.columnMin!==void 0&&this.columnMin===this.columnMax,r=e||this.rowMin!==void 0&&this.rowMin===this.rowMax,o=this.rowMin??1,n=this.rowMax??this.rows,s=Math.ceil((this.columnMin??1)/this.step)*this.step,a=Math.ceil((this.columnMax??this.columns)/this.step)*this.step,l=e?o:(g=this._localValue)==null?void 0:g.rows,c=(_=this._localValue)==null?void 0:_.columns,d=e?1:Number((k=this._localValue)==null?void 0:k.rows)||1,h=t?this.columns:Number((S=this._localValue)==null?void 0:S.columns)||1;return u`
      <div
        class="grid"
        style=${T({"--preview-rows":String(this.rows)})}
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
          ${this.isDefault?m:u`
                <ha-icon-button
                  .path=${ap}
                  label="恢复默认"
                  @click=${this._reset}
                ></ha-icon-button>
              `}
        </div>

        <div
          class="preview"
          style=${T({"--rows":String(d),"--columns":String(h),"--total-columns":String(this.columns)})}
        >
          <table>
            ${Array(this.rows).fill(0).map((M,N)=>{const R=N+1;return u`
                  <tr>
                    ${Array(this.columns).fill(0).map((Q,B)=>{const J=B+1;return J%this.step!==0||this.columns>24&&J%3!==0?m:u`
                          <td
                            data-row=${R}
                            data-column=${J}
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
    `}};Pe.styles=$`
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
  `;it([p({attribute:!1})],Pe.prototype,"value",2);it([p({type:Number})],Pe.prototype,"rows",2);it([p({type:Number})],Pe.prototype,"columns",2);it([p({type:Number})],Pe.prototype,"rowMin",2);it([p({type:Number})],Pe.prototype,"rowMax",2);it([p({type:Number})],Pe.prototype,"columnMin",2);it([p({type:Number})],Pe.prototype,"columnMax",2);it([p({type:Boolean})],Pe.prototype,"isDefault",2);it([p({type:Number})],Pe.prototype,"step",2);it([b()],Pe.prototype,"_localValue",2);Pe=it([w("ha-grid-size-picker")],Pe);var zx=Object.defineProperty,Nx=Object.getOwnPropertyDescriptor,Ko=(e,t,i,r)=>{for(var o=r>1?void 0:r?Nx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&zx(t,i,o),o};let Ii=class extends y{constructor(){super(...arguments),this._preciseMode=!1}async _loadDefaults(){var e;try{const t=await ul(this.config);this._defaultGridOptions=((e=t.getGridOptions)==null?void 0:e.call(t))??{}}catch{this._defaultGridOptions={}}}firstUpdated(){this._loadDefaults()}willUpdate(e){if(e.has("config")){const t=this.config.grid_options;if(!t){this._preciseMode=this._defaultGridOptions?Tc(this._defaultGridOptions):!1;return}const i=Tc(t);!this._preciseMode&&i&&(this._preciseMode=i)}}updated(e){e.has("config")&&this._loadDefaults()}_configGridOptions(e){return e.grid_options?e.grid_options:e.layout_options?Th(e.layout_options):{}}_mergedOptions(e,t){return{...ir,...t,...e}}_isDefault(e){return(e==null?void 0:e.columns)===void 0&&(e==null?void 0:e.rows)===void 0}_updateGridOptions(e){const t={...this.config,grid_options:{...e}};if(t.grid_options){for(const[i,r]of Object.entries(t.grid_options))r===void 0&&delete t.grid_options[i];Object.keys(t.grid_options).length===0&&delete t.grid_options}t.layout_options&&delete t.layout_options,this.dispatchEvent(new CustomEvent("config-changed",{bubbles:!0,composed:!0,detail:{config:t}}))}_gridSizeChanged(e){e.stopPropagation();const t=e.detail.value;this._updateGridOptions({...this.config.grid_options,columns:t.columns,rows:t.rows})}_fullWidthChanged(e){var o;e.stopPropagation();const t=e.target.checked,i={...ir,...this._defaultGridOptions};let r;if(t)r="full";else if(i.columns==="full"){const s=12*(((o=this.sectionConfig)==null?void 0:o.column_span)??1);r=i.max_columns??s}else r=void 0;this._updateGridOptions({...this.config.grid_options,columns:r})}_autoHeightChanged(e){e.stopPropagation();const t=e.target.checked,i={...ir,...this._defaultGridOptions};let r;t?r="auto":i.rows==="auto"?r=i.min_rows??1:r=void 0,this._updateGridOptions({...this.config.grid_options,rows:r})}_preciseModeChanged(e){var i;if(e.stopPropagation(),this._preciseMode=e.target.checked,this._preciseMode)return;const t=(i=this.config.grid_options)==null?void 0:i.columns;if(typeof t=="number"&&t%Vt!==0){const r=Math.ceil(t/Vt)*Vt;this._updateGridOptions({...this.config.grid_options,columns:r})}}render(){var l;const e=this._configGridOptions(this.config),t=this._mergedOptions(e,this._defaultGridOptions),i=Oh(t),o=12*(((l=this.sectionConfig)==null?void 0:l.column_span)??1),n=t.rows==="auto",s=t.columns==="full",a=this._defaultGridOptions!==void 0&&Object.keys(this._defaultGridOptions).length===0;return u`
      ${a?u`
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
        .step=${this._preciseMode?1:Vt}
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
    `}};Ii.styles=$`
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
  `;Ko([p({attribute:!1})],Ii.prototype,"config",2);Ko([p({attribute:!1})],Ii.prototype,"sectionConfig",2);Ko([b()],Ii.prototype,"_defaultGridOptions",2);Ko([b()],Ii.prototype,"_preciseMode",2);Ii=Ko([w("flow-card-layout-editor")],Ii);var Rx=Object.defineProperty,Lx=Object.getOwnPropertyDescriptor,Hi=(e,t,i,r)=>{for(var o=r>1?void 0:r?Lx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Rx(t,i,o),o};let At=class extends y{constructor(){super(...arguments),this.open=!1,this._draft={type:"tile"},this._tab="config",this._layoutRevision=0,this._backdropDismiss=Mr(()=>this._close())}updated(e){e.has("open")&&this.open&&(this._tab="config"),e.has("config")&&this.config&&(this._draft=structuredClone(this.config),this._layoutRevision+=1)}_close(){this.dispatchEvent(new CustomEvent("close",{bubbles:!0,composed:!0}))}_save(){this.dispatchEvent(new CustomEvent("save",{bubbles:!0,composed:!0,detail:{config:structuredClone(this._draft)}}))}_onConfigChanged(e){e.stopPropagation(),this._draft=e.detail.config,this._layoutRevision+=1}_onPicturePosition(e){if(e.stopPropagation(),this._tab!=="config")return;const t=this.renderRoot.querySelector("flow-card-config-editor");t==null||t.applyPictureElementPosition(e.detail.left,e.detail.top)}_onPictureElementsChanged(e){e.stopPropagation(),this._draft=e.detail.config,this._layoutRevision+=1}_previewSectionConfig(){const e=this.sectionConfig;if(e){const{cards:t,title:i,...r}=e;return{...r,type:e.type??"grid",column_span:1,cards:[this._draft]}}return{type:"grid",column_span:1,cards:[this._draft]}}_cardTypeName(){return{tile:"Toggle",heading:"Title",sensor:"数值",switch:"开关",trigger:"触发器",climate:"步进",cover:"单选",clock:"Clock",slider:"滑块",gain:"增益",media:"媒体",markdown:"Markdown",link:"超链接","picture-elements":"图片元素",line2d:"2D 折线",scatter2d:"2D 散点",bar:"柱状图",line3d:"3D 折线",scatter3d:"3D 散点"}[String(this._draft.type)]??String(this._draft.type)}render(){if(!this.open)return m;const e=this._previewSectionConfig();return u`
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
              ${this._tab==="config"?u`
                    <flow-card-config-editor
                      .config=${this._draft}
                      @config-changed=${this._onConfigChanged}
                    ></flow-card-config-editor>
                  `:u`
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
    `}};At.styles=$`
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
  `;Hi([Tr({context:zi,subscribe:!0}),p({attribute:!1})],At.prototype,"flow",2);Hi([p({type:Boolean})],At.prototype,"open",2);Hi([p({attribute:!1})],At.prototype,"config",2);Hi([p({attribute:!1})],At.prototype,"sectionConfig",2);Hi([b()],At.prototype,"_draft",2);Hi([b()],At.prototype,"_tab",2);At=Hi([w("flow-card-editor")],At);class Bx{constructor(t=2){this.maxConcurrent=t,this.active=0,this.pending=[]}enqueue(t){this.pending.push(t),this.pump()}pump(){for(;this.active<this.maxConcurrent&&this.pending.length>0;){const t=this.pending.shift();if(!t)return;this.active+=1,t().finally(()=>{this.active-=1,this.pump()})}}}const Fx=new Bx(3);var Hx=Object.defineProperty,Ux=Object.getOwnPropertyDescriptor,fe=(e,t,i,r)=>{for(var o=r>1?void 0:r?Ux(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Hx(t,i,o),o};const gn=6;function jx(e,t){const i=t.trim().toLowerCase();return i?e.filter(r=>[r.type,r.name,r.description??""].join(" ").toLowerCase().includes(i)):e}let Je=class extends y{constructor(){super(...arguments),this.type="",this.name="",this.icon="mdi:card-outline",this.index=0,this._previewState="placeholder",this._loadRequested=!1,this._disconnected=!1}connectedCallback(){super.connectedCallback(),this._disconnected=!1,this.index<gn&&this._scheduleVisibilityCheck(),this._bindScrollRoot()}disconnectedCallback(){this._disconnected=!0,this._teardownObserver(),super.disconnectedCallback()}updated(e){e.has("scrollRoot")&&this._bindScrollRoot(),(e.has("config")||e.has("type"))&&(this._loadRequested=!1,this._card=void 0,this._previewState="placeholder",this._scheduleVisibilityCheck()),e.has("index")&&this.index<gn&&this._scheduleVisibilityCheck(),this._previewState==="ready"&&this._card&&requestAnimationFrame(()=>this._mountPreview())}_teardownObserver(){var e;(e=this._observer)==null||e.disconnect(),this._observer=void 0,this._scrollListener&&this.scrollRoot&&this.scrollRoot.removeEventListener("scroll",this._scrollListener),this._scrollListener=void 0}_bindScrollRoot(){this._teardownObserver(),this.scrollRoot&&(this._observer=new IntersectionObserver(e=>{e.some(t=>t.isIntersecting)&&this._requestPreview()},{root:this.scrollRoot,rootMargin:"100px 0px",threshold:.01}),this._observer.observe(this),this._scrollListener=()=>this._scheduleVisibilityCheck(),this.scrollRoot.addEventListener("scroll",this._scrollListener,{passive:!0}),this._scheduleVisibilityCheck())}_scheduleVisibilityCheck(){requestAnimationFrame(()=>{this._disconnected||this._loadRequested||(this.index<gn||this._isVisibleInScrollRoot())&&this._requestPreview()})}_isVisibleInScrollRoot(){if(!this.scrollRoot)return this.index<gn;const e=this.getBoundingClientRect();if(e.width<=0||e.height<=0)return!1;const t=this.scrollRoot.getBoundingClientRect();return e.bottom>t.top&&e.top<t.bottom}_requestPreview(){this._loadRequested||(this._loadRequested=!0,this._teardownObserver(),this._previewState!=="ready"&&(this._previewState="loading",Fx.enqueue(()=>this._loadPreview())))}async _loadPreview(){var e,t;try{const i=((e=this.config)==null?void 0:e.type)||this.type,r=await ya(i);if(this._disconnected)return;if(!r){this._previewState="error";return}const o=this.config??((t=r.getStubConfig)==null?void 0:t.call(r))??{type:i},n=await ul(o);if(this._disconnected)return;n.flow=this.flow,this._card=n,this._previewState="ready"}catch{this._disconnected||(this._previewState="error")}}_mountPreview(){if(!this._card||this._previewState!=="ready")return;this._card.flow=this.flow;const e=this.renderRoot.querySelector(".preview-inner");e&&!e.contains(this._card)&&e.replaceChildren(this._card)}_pick(){var e;this.dispatchEvent(new CustomEvent("card-select",{detail:{type:((e=this.config)==null?void 0:e.type)||this.type,config:this.config?structuredClone(this.config):void 0},bubbles:!0,composed:!0}))}render(){return u`
      <button type="button" class="card" @click=${this._pick}>
        <div class="card-header">${this.name}</div>
        <div class="preview">
          ${this._previewState==="ready"?u`<div class="preview-inner"></div>`:this._previewState==="loading"?u`<div class="loading" aria-label="加载预览"></div>`:u`
                  <div class="preview-icon">
                    <ha-icon .icon=${this.icon}></ha-icon>
                  </div>
                `}
        </div>
      </button>
    `}};Je.styles=$`
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
  `;fe([p()],Je.prototype,"type",2);fe([p()],Je.prototype,"name",2);fe([p()],Je.prototype,"icon",2);fe([p({attribute:!1})],Je.prototype,"config",2);fe([p({type:Number})],Je.prototype,"index",2);fe([p({attribute:!1})],Je.prototype,"flow",2);fe([p({attribute:!1})],Je.prototype,"scrollRoot",2);fe([b()],Je.prototype,"_previewState",2);Je=fe([w("flow-card-picker-item")],Je);let Qt=class extends y{constructor(){super(...arguments),this.open=!1,this._query="",this._backdropDismiss=Mr(()=>this._close())}connectedCallback(){super.connectedCallback(),this._refreshClipboard(),this._unsubClipboard=Ub(()=>this._refreshClipboard())}disconnectedCallback(){var e;(e=this._unsubClipboard)==null||e.call(this),super.disconnectedCallback()}firstUpdated(){this._syncScrollRoot()}updated(e){e.has("open")&&this.open&&(this._query="",this._refreshClipboard(),this._syncScrollRoot())}_refreshClipboard(){this._clipboard=Fb()}_syncScrollRoot(){const e=this.renderRoot.querySelector(".body");e&&e!==this._scrollRoot&&(this._scrollRoot=e)}_close(){this.dispatchEvent(new CustomEvent("close",{bubbles:!0,composed:!0}))}_onCardSelect(e){e.stopPropagation(),this.dispatchEvent(new CustomEvent("card-picked",{detail:e.detail,bubbles:!0,composed:!0}))}_onSearchInput(e){this._query=e.target.value}_allCards(){return mp()}_filteredCards(){return jx(this._allCards(),this._query)}_suggestedEntries(e=fc){const t=this._allCards(),i=new Map(t.map(a=>[a.type,a])),r=uh().map(a=>i.get(a)).filter(a=>!!a),o=Vb.map(a=>i.get(a)).filter(a=>!!a),n=new Set,s=[];for(const a of[...r,...o])if(!n.has(a.type)&&(n.add(a.type),s.push(a),s.length>=e))break;return s}_renderItem(e,t,i){return u`
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
    `}_renderGrid(e,t=0,i){return e.length?u`
      <div class="cards">
        ${e.map((r,o)=>{var n,s;return this._renderItem(r,t+o,{config:(n=i==null?void 0:i.configForType)==null?void 0:n.call(i,r.type),name:(s=i==null?void 0:i.nameForType)==null?void 0:s.call(i,r.type)})})}
      </div>
    `:u`<div class="empty">没有匹配的卡片</div>`}_renderClipboardCard(e){const t=this._clipboard;if(!t)return m;const i=fp(t.type)??{type:t.type,name:t.type,icon:"mdi:content-paste",category:"container"};return this._renderItem(i,e,{config:t,name:"从剪贴板粘贴"})}_renderSections(){if(this._query.trim())return this._renderGrid(this._filteredCards());const t=!!this._clipboard,i=this._suggestedEntries(Math.max(0,fc-(t?1:0))),r=this._allCards();let o=0;return u`
      ${t||i.length?u`
            <ha-expandable-panel .header=${"推荐卡片"} expanded>
              <div class="cards">
                ${t?this._renderClipboardCard(o++):m}
                ${i.map(n=>this._renderItem(n,o++))}
              </div>
            </ha-expandable-panel>
          `:m}

      <ha-expandable-panel .header=${"核心卡"} expanded>
        ${!t&&!i.length?u`<div class="cards">${this._renderClipboardCard(0)}</div>`:m}
        ${this._renderGrid(r,o)}
      </ha-expandable-panel>
    `}render(){return this.open?u`
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
              .path=${lp}
              label="关闭"
              @click=${this._close}
            ></ha-icon-button>
            <h2>添加到仪表板</h2>
          </header>

          <div class="search-wrap">
            <label class="search">
              <ha-svg-icon class="search-icon" .path=${td}></ha-svg-icon>
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
    `:m}};Qt.styles=$`
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
  `;fe([p({type:Boolean,reflect:!0})],Qt.prototype,"open",2);fe([p({attribute:!1})],Qt.prototype,"flow",2);fe([b()],Qt.prototype,"_query",2);fe([b()],Qt.prototype,"_scrollRoot",2);fe([b()],Qt.prototype,"_clipboard",2);Qt=fe([w("flow-card-picker")],Qt);var Vx=Object.defineProperty,qx=Object.getOwnPropertyDescriptor,oi=(e,t,i,r)=>{for(var o=r>1?void 0:r?qx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Vx(t,i,o),o};let ft=class extends y{constructor(){super(...arguments),this.open=!1,this.maxColumns=4,this._columnSpan=1,this._backgroundEnabled=!1,this._backgroundColor="default",this._backgroundOpacity=_n,this._backdropDismiss=Mr(()=>this._close())}updated(e){if(e.has("config")&&this.config){this._columnSpan=this.config.column_span??1;const t=this.config.background;this._backgroundEnabled=t!==void 0,typeof t=="object"&&t?(this._backgroundColor=t.color??"default",this._backgroundOpacity=t.opacity??_n):(this._backgroundColor="default",this._backgroundOpacity=_n)}}_pickerColor(){return this._backgroundColor==="default"?"":this._backgroundColor}_close(){this.dispatchEvent(new CustomEvent("close",{bubbles:!0,composed:!0}))}_save(){const e={column_span:this._columnSpan};if(this._backgroundEnabled){const t={opacity:this._backgroundOpacity};this._backgroundColor!=="default"&&(t.color=this._backgroundColor),e.background=t}else e.background=void 0;this.dispatchEvent(new CustomEvent("save",{bubbles:!0,composed:!0,detail:{patch:e}}))}render(){return this.open?u`
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

          ${this._backgroundEnabled?u`
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
    `:m}};ft.styles=$`
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
  `;oi([p({type:Boolean})],ft.prototype,"open",2);oi([p({attribute:!1})],ft.prototype,"config",2);oi([p({type:Number})],ft.prototype,"maxColumns",2);oi([b()],ft.prototype,"_columnSpan",2);oi([b()],ft.prototype,"_backgroundEnabled",2);oi([b()],ft.prototype,"_backgroundColor",2);oi([b()],ft.prototype,"_backgroundOpacity",2);ft=oi([w("flow-section-editor")],ft);var Gx=Object.defineProperty,Xx=Object.getOwnPropertyDescriptor,qe=(e,t,i,r)=>{for(var o=r>1?void 0:r?Xx(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&Gx(t,i,o),o};const Yx=/^[a-zA-Z0-9_-]+$/,su=/^[0-9]+$/;function Wc(e){const t=e.trim().toLowerCase().normalize("NFKD").replace(/[\u0300-\u036f]/g,"").replace(/[^a-z0-9]+/g,"-").replace(/^-+|-+$/g,"");return t?su.test(t)?`view-${t}`:t:""}let xe=class extends y{constructor(){super(...arguments),this.open=!1,this.isNew=!1,this.reservedPaths=[],this._title="",this._icon="mdi:view-dashboard",this._path="",this._maxColumns=4,this._densePlacement=!1,this._pathError="",this._suggestedPath=!1,this._backdropDismiss=Mr(()=>this._close())}updated(e){if((e.has("open")||e.has("config"))&&this.open){const t=this.config;this._title=(t==null?void 0:t.title)??(this.isNew?"新建视图":""),this._icon=(t==null?void 0:t.icon)||"mdi:view-dashboard",this._path=(t==null?void 0:t.path)??"",this._maxColumns=Math.max(1,Math.min(10,Number(t==null?void 0:t.max_columns)||4)),this._densePlacement=!!(t!=null&&t.dense_section_placement),this._pathError="",this._suggestedPath=!1,this.isNew&&!this._path&&(this._path=this._uniquePath(Wc(this._title)||"view"),this._suggestedPath=!0),this._validatePath(this._path)}}_uniquePath(e){const t=new Set(this.reservedPaths);if(!t.has(e)&&e)return e;let i=2,r=`${e||"view"}-${i}`;for(;t.has(r);)i+=1,r=`${e||"view"}-${i}`;return r}_validatePath(e){return e?Yx.test(e)?su.test(e)?(this._pathError="不能是纯数字（会与视图索引冲突）",!1):this.reservedPaths.includes(e)?(this._pathError="该路径已被其他视图占用",!1):(this._pathError="",!0):(this._pathError="仅允许字母、数字、下划线和连字符",!1):(this._pathError="网址路径不能为空",!1)}_onTitleInput(e){const t=e.target.value;if(this._title=t,this.isNew&&this._suggestedPath){const i=Wc(t);this._path=this._uniquePath(i||"view"),this._validatePath(this._path)}}_onPathInput(e){this._suggestedPath=!1,this._path=e.target.value.trim(),this._validatePath(this._path)}_close(){this.dispatchEvent(new CustomEvent("close",{bubbles:!0,composed:!0}))}_save(){var t;if(!this._validatePath(this._path))return;const e={...this.config??{sections:[]},title:this._title.trim()||"新建视图",icon:this._icon||"mdi:view-dashboard",path:this._path,max_columns:this._maxColumns,dense_section_placement:this._densePlacement||void 0,sections:((t=this.config)==null?void 0:t.sections)??[]};e.dense_section_placement||delete e.dense_section_placement,this.dispatchEvent(new CustomEvent("save",{bubbles:!0,composed:!0,detail:{config:e,isNew:this.isNew}}))}render(){if(!this.open)return m;const e=!this._pathError&&!!this._path;return u`
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
            ${this._pathError?u`<span class="error">${this._pathError}</span>`:u`<span class="helper">该值将成为打开此视图的网址路径的一部分（如 #/${this._path||"…"}）。</span>`}
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
    `}};xe.styles=$`
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
  `;qe([p({type:Boolean})],xe.prototype,"open",2);qe([p({type:Boolean})],xe.prototype,"isNew",2);qe([p({attribute:!1})],xe.prototype,"config",2);qe([p({attribute:!1})],xe.prototype,"reservedPaths",2);qe([b()],xe.prototype,"_title",2);qe([b()],xe.prototype,"_icon",2);qe([b()],xe.prototype,"_path",2);qe([b()],xe.prototype,"_maxColumns",2);qe([b()],xe.prototype,"_densePlacement",2);qe([b()],xe.prototype,"_pathError",2);qe([b()],xe.prototype,"_suggestedPath",2);xe=qe([w("flow-view-editor")],xe);async function Wx(){try{const e=await fetch("/api/info/app");if(!e.ok)return{name:"Flow",version:"未知"};const t=await e.json();return t.ok?{name:t.name?String(t.name):"Flow",version:t.version?String(t.version):"未知"}:{name:"Flow",version:"未知"}}catch{return{name:"Flow",version:"获取失败"}}}async function Kx(){try{const e=await fetch("/api/info/current_flow");if(!e.ok)return"获取失败";const t=await e.json();return t.ok&&t.filename?String(t.filename):"无项目运行"}catch{return"连接异常"}}function Kc(e,t,i){return new Promise((r,o)=>{if(!t||typeof t.size!="number"){o(new Error("invalid_file"));return}const n=new XMLHttpRequest;n.open("POST",e,!0),n.setRequestHeader("Content-Type","application/octet-stream"),n.upload.onprogress=s=>{i&&(s.lengthComputable&&s.total>0?i(Math.round(s.loaded/s.total*100)):t.size>0&&i(Math.min(99,Math.round(s.loaded/t.size*100))))},n.onload=()=>{const s=n.responseText||"";let a=null;try{a=JSON.parse(s)}catch{a=null}if(n.status>=200&&n.status<300&&(a!=null&&a.ok)){i==null||i(100),r({ok:!0,path:a.path||""});return}o(new Error((a==null?void 0:a.error)??`${n.status} ${s}`))},n.onerror=()=>o(new Error("network_error")),n.onabort=()=>o(new Error("aborted")),n.send(t)})}async function Zx(){const e=await fetch("/api/download/current_flow");if(!e.ok){const n=await e.json().catch(()=>({}));throw new Error(n.error||e.statusText)}const t=await e.blob(),i=e.headers.get("Content-Disposition")||"";let r="project.flow";const o=/filename="([^"]*)"/.exec(i);return o!=null&&o[1]&&(r=o[1]),{filename:r,blob:t}}const Qx="video/*,audio/*,image/*,.mp4,.mov,.avi,.mkv,.mp3,.wav,.png,.jpg,.jpeg,.gif",au="ns_setting_auth_ts",Jx=10*60*1e3;function Zc(){try{sessionStorage.setItem(au,String(Date.now()))}catch{}}function ew(){try{const e=Number(sessionStorage.getItem(au)||0);return e>0&&Date.now()-e<Jx}catch{return!1}}async function lu(e){try{const t=await fetch("/api/auth/setting",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({password:e||""})}),i=await t.json().catch(()=>null);return t.ok&&(i!=null&&i.ok)?"ok":"deny"}catch{return"offline"}}async function tw(e){const t=await lu(e);return t==="ok"||t==="offline"}var iw=Object.defineProperty,rw=Object.getOwnPropertyDescriptor,Lr=(e,t,i,r)=>{for(var o=r>1?void 0:r?rw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&iw(t,i,o),o};let Jt=class extends y{constructor(){super(...arguments),this._entity="",this._name="",this._error="",this._adding=!1,this._success=""}_reset(){this._entity="",this._name="",this._error=""}async _submit(){const e=this._entity.trim();if(!e){this._error="请输入控制地址";return}if(!e.startsWith("/")){this._error="控制地址应以 / 开头";return}this._adding=!0,this._error="",this._success="";const t=this._name.trim()||e,i=await yy(e,t);if(this._adding=!1,!i){this._error="添加失败，请检查地址是否有效";return}this._success=`已添加：${nt(i)}`,this._reset()}render(){return u`
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
            placeholder="留空则使用控制地址"
            ?disabled=${this._adding}
            @input=${e=>{this._name=e.target.value}}
          />
        </label>
        ${this._error?u`<p class="error">${this._error}</p>`:m}
        ${this._success?u`<p class="success">${this._success}</p>`:m}
        <div class="actions">
          <button
            type="button"
            class="btn primary"
            ?disabled=${this._adding}
            @click=${()=>void this._submit()}
          >
            ${this._adding?"添加中…":"添加到动作库"}
          </button>
        </div>
      </div>
    `}};Jt.styles=$`
    :host {
      display: block;
    }
    .help {
      margin: 0 0 12px;
      font-size: 13px;
      line-height: 1.45;
      color: var(--secondary-text-color);
    }
    .form {
      display: flex;
      flex-direction: column;
      gap: 0;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
    }
    .label {
      font-size: 13px;
      color: var(--secondary-text-color);
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
    .error {
      margin: -4px 0 8px;
      font-size: 12px;
      color: var(--error-color, #db4437);
    }
    .success {
      margin: -4px 0 8px;
      font-size: 12px;
      color: var(--success-color, #0f9d58);
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 8px;
      flex-wrap: wrap;
    }
    .btn {
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      background: #fff;
      color: var(--primary-text-color);
      font: inherit;
      font-size: 13px;
      font-weight: 600;
      padding: 8px 14px;
      cursor: pointer;
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
  `;Lr([b()],Jt.prototype,"_entity",2);Lr([b()],Jt.prototype,"_name",2);Lr([b()],Jt.prototype,"_error",2);Lr([b()],Jt.prototype,"_adding",2);Lr([b()],Jt.prototype,"_success",2);Jt=Lr([w("flow-manual-action-form")],Jt);var ow=Object.defineProperty,nw=Object.getOwnPropertyDescriptor,W=(e,t,i,r)=>{for(var o=r>1?void 0:r?nw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&ow(t,i,o),o};const sw=2;let H=class extends y{constructor(){super(...arguments),this._loading=!0,this._authed=!1,this._passwordRequired=!0,this._authPassword="",this._authError="",this._authLoading=!1,this._layoutEditEnabled=!1,this._appName="Flow",this._appVersion="…",this._currentFlowName="检测中...",this._mediaFiles=[],this._flowFile=null,this._mediaStatus="",this._flowStatus="",this._downloadStatus="",this._mediaUploading=!1,this._flowUploading=!1,this._downloading=!1,this._mediaProgress={show:!1,percent:0,label:""},this._flowProgress={show:!1,percent:0},this._columnCount=1,this._mediaDragOver=!1}connectedCallback(){super.connectedCallback(),this._resizeObserver=new ResizeObserver(e=>{var n;const t=((n=e[0])==null?void 0:n.contentRect.width)??0;if(!t)return;const i=320,r=32,o=Math.max(1,Math.min(sw,Math.floor((t+r)/(i+r))));o!==this._columnCount&&(this._columnCount=o)}),this._resizeObserver.observe(this),this._layoutEditEnabled=ch(),this._loadMeta()}disconnectedCallback(){var e;(e=this._resizeObserver)==null||e.disconnect(),super.disconnectedCallback()}_gridStyle(){return`--column-count: ${this._columnCount}`}async _loadMeta(){this._loading=!0;try{const e=await Wx();this._appName=e.name,this._appVersion=e.version,this._currentFlowName=await Kx(),await this._refreshAuthState()}finally{this._loading=!1}}async _refreshAuthState(){if(ew()){this._authed=!0;return}const e=await lu("");if(e==="ok"||e==="offline"){Zc(),this._authed=!0,this._passwordRequired=!1;return}this._authed=!1,this._passwordRequired=!0}async _submitAuth(){this._authLoading=!0,this._authError="";try{if(await tw(this._authPassword)){Zc(),this._authed=!0,this._authPassword="";return}this._authError="密码错误，请重试"}finally{this._authLoading=!1}}_onAuthKeydown(e){e.key==="Enter"&&this._submitAuth()}_toggleLayoutEdit(e){if(!this._authed)return;const t=e.target.checked;this._layoutEditEnabled=t,Lb(t)}_setMediaFiles(e){const t=Array.from(e).map(i=>({file:i,status:"ready"}));this._mediaFiles=t}_onMediaInput(e){var i;const t=e.target;(i=t.files)!=null&&i.length&&this._setMediaFiles(t.files),t.value=""}_onMediaDrop(e){var t,i;e.preventDefault(),this._mediaDragOver=!1,(i=(t=e.dataTransfer)==null?void 0:t.files)!=null&&i.length&&this._setMediaFiles(e.dataTransfer.files)}_removeMedia(e){this._mediaFiles=this._mediaFiles.filter((t,i)=>i!==e)}_onFlowInput(e){var r;const t=e.target,i=(r=t.files)==null?void 0:r[0];this._flowFile=i?{file:i,status:"ready"}:null,t.value=""}async _uploadMedia(){const e=this._mediaFiles.map(i=>i.file);if(!e.length){this._mediaStatus="请选择媒体文件";return}this._mediaUploading=!0,this._mediaProgress={show:!0,percent:0,label:""};const t=[];try{for(let o=0;o<e.length;o++){const n=e[o],s=`${o+1}/${e.length} ${n.name}`;this._mediaProgress={...this._mediaProgress,label:s},this._mediaStatus=`正在上传：${s}`;const a=`/api/upload/media?filename=${encodeURIComponent(n.name)}`;try{await Kc(a,n,l=>{const c=o/e.length*100;this._mediaProgress={show:!0,label:s,percent:Math.min(100,Math.round(c+l/e.length))}}),t.push({name:n.name,ok:!0}),this._mediaFiles=this._mediaFiles.map((l,c)=>c===o?{...l,status:"success"}:l)}catch(l){const c=l instanceof Error?l.message:String(l);t.push({name:n.name,ok:!1,error:c}),this._mediaFiles=this._mediaFiles.map((d,h)=>h===o?{...d,status:"fail"}:d)}}this._mediaProgress={...this._mediaProgress,percent:100};const i=t.filter(o=>o.ok).length,r=t.length-i;this._mediaStatus=`完成：成功 ${i}，失败 ${r}`+(r?`；失败：${t.filter(o=>!o.ok).map(o=>`${o.name}(${o.error})`).join("；")}`:"")}catch(i){this._mediaStatus=`上传异常：${i}`}finally{this._mediaUploading=!1}}async _uploadFlow(){const e=this._flowFile,t=e==null?void 0:e.file;if(!t){this._flowStatus="请选择.flow文件";return}if(!t.name.toLowerCase().endsWith(".flow")){this._flowStatus="文件扩展名必须为 .flow";return}this._flowUploading=!0,this._flowProgress={show:!0,percent:0},this._flowStatus="开始上传...";try{const i=`/api/upload/flow?filename=${encodeURIComponent(t.name)}`,r=await Kc(i,t,o=>{this._flowProgress={show:!0,percent:o}});this._flowProgress={show:!0,percent:100},this._flowStatus=`上传成功：${r.path}`,this._flowFile={file:t,status:"success"}}catch(i){this._flowStatus=`上传失败：${i instanceof Error?i.message:i}`,this._flowFile={file:t,status:"fail"}}finally{this._flowUploading=!1}}async _downloadFlow(){this._downloadStatus="正在请求...",this._downloading=!0;try{const{filename:e,blob:t}=await Zx(),i=URL.createObjectURL(t),r=document.createElement("a");r.href=i,r.download=e,document.body.appendChild(r),r.click(),r.remove(),URL.revokeObjectURL(i),this._downloadStatus="下载完成"}catch(e){this._downloadStatus=`下载失败：${e instanceof Error?e.message:e}`}finally{this._downloading=!1}}_renderUploadIcon(){return u`
      <div class="upload-icon" aria-hidden="true">
        <svg viewBox="0 0 1024 1024" xmlns="http://www.w3.org/2000/svg">
          <path
            fill="currentColor"
            d="M544 864V672h128L512 480 352 672h128v192H544zM160 704a32 32 0 0 1-32-32V224a64 64 0 0 1 64-64h640a64 64 0 0 1 64 64v448a32 32 0 0 1-64 0V224H192v448a32 32 0 0 1-32 32zm704 64H704v-64h160a32 32 0 1 1 0 64zM160 768h160v64H160a32 32 0 0 1 0-64z"
          />
        </svg>
      </div>
    `}_renderAuthCard(){return u`
      <div class="grid-slot">
        <ha-card>
          <div class="card-body">
            <h2 class="card-title">访问验证</h2>
            ${this._authed?u`
                  <p class="help auth-ok">已通过验证，可使用下方设置项。</p>
                  <span class="tag success">已解锁</span>
                `:u`
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
                  ${this._authError?u`<div class="alert error">${this._authError}</div>`:m}
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
    `}_renderLockedSlot(e){return u`
      <div class="grid-slot ${this._authed?"":"locked"}">
        <div class="card-lock-wrap" ?inert=${!this._authed}>${e}</div>
      </div>
    `}_renderMain(){return u`
      <div class="wrapper" style=${this._gridStyle()}>
        <div class="grid">
          ${this._passwordRequired?this._renderAuthCard():m}

          ${this._renderLockedSlot(u`
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

          ${this._renderLockedSlot(u`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">上传媒体文件</h2>
                <p class="help">将文件拖拽到下方区域，或点击选择。支持多文件。</p>
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
                    accept=${Qx}
                    ?disabled=${this._mediaUploading}
                    @change=${this._onMediaInput}
                  />
                </div>
                <p class="upload-tip">支持常见音视频 / 图片，可一次选择多个文件</p>
                ${this._mediaFiles.length?u`
                      <ul class="file-list">
                        ${this._mediaFiles.map((e,t)=>u`
                            <li class="file-item ${e.status??""}">
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
                    @click=${this._uploadMedia}
                  >
                    ${this._mediaUploading?"上传中...":"上传媒体"}
                  </button>
                  <span class="status">${this._mediaStatus}</span>
                </div>
                ${this._mediaProgress.show?u`
                      <div class="progress-wrap">
                        <div class="progress-bar">
                          <div
                            class="progress-fill"
                            style=${`width:${this._mediaProgress.percent}%`}
                          ></div>
                        </div>
                        <span class="progress-label">
                          ${this._mediaProgress.label||`${this._mediaProgress.percent}%`}
                        </span>
                      </div>
                    `:m}
              </div>
            </ha-card>
          `)}

          ${this._renderLockedSlot(u`
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
                ${this._flowFile?u`<div class="file-chip ${this._flowFile.status??""}">${this._flowFile.file.name}</div>`:m}
                ${this._flowStatus?u`<div class="status block">${this._flowStatus}</div>`:m}
                ${this._flowProgress.show?u`
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

          ${this._renderLockedSlot(u`
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

          ${this._renderLockedSlot(u`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">动作库</h2>
                <flow-manual-action-form></flow-manual-action-form>
              </div>
            </ha-card>
          `)}

          ${this._renderLockedSlot(u`
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
    `}render(){return u`
      <div class="page">
        ${this._loading?u`<div class="wrapper" style=${this._gridStyle()}>
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
    `}};H.styles=$`
    :host {
      --row-gap: var(--ha-view-sections-row-gap, 24px);
      --column-gap: var(--ha-view-sections-column-gap, 32px);
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
      margin: var(--ha-view-sections-extra-top-margin, 24px) auto 0;
      max-width: calc(
        var(--column-count) * var(--column-max-width) +
          (var(--column-count) - 1) * var(--column-gap)
      );
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(var(--column-count), 1fr);
      gap: var(--row-gap) var(--column-gap);
      padding: var(--row-gap) 0 calc(var(--row-gap) * 2);
      align-items: start;
    }
    .grid-slot {
      min-width: 0;
    }
    .grid-slot.locked .card-lock-wrap {
      opacity: 0.42;
      pointer-events: none;
      user-select: none;
      filter: grayscale(0.35);
    }
    .auth-ok {
      margin-bottom: 8px;
    }
    .text-input {
      width: 100%;
      box-sizing: border-box;
      height: 40px;
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
      margin-top: 12px;
      padding: 10px 12px;
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
      padding: 12px 0 4px;
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
      padding: 20px;
    }
    .card-title {
      margin: 0 0 12px;
      font-size: 16px;
      font-weight: 600;
    }
    .checking {
      text-align: center;
      padding: 12px 0;
      color: #64748b;
    }
    .help {
      margin: 0 0 12px;
      color: var(--secondary-text-color);
      font-size: 13px;
      line-height: 1.5;
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 12px;
      flex-wrap: wrap;
      margin-top: 12px;
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
      padding: 28px 16px;
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
      font-size: 48px;
      color: #c0c4cc;
      margin-bottom: 8px;
      line-height: 1;
    }
    .upload-icon svg {
      width: 48px;
      height: 48px;
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
  `;W([p({attribute:!1})],H.prototype,"flow",2);W([b()],H.prototype,"_loading",2);W([b()],H.prototype,"_authed",2);W([b()],H.prototype,"_passwordRequired",2);W([b()],H.prototype,"_authPassword",2);W([b()],H.prototype,"_authError",2);W([b()],H.prototype,"_authLoading",2);W([b()],H.prototype,"_layoutEditEnabled",2);W([b()],H.prototype,"_appName",2);W([b()],H.prototype,"_appVersion",2);W([b()],H.prototype,"_currentFlowName",2);W([b()],H.prototype,"_mediaFiles",2);W([b()],H.prototype,"_flowFile",2);W([b()],H.prototype,"_mediaStatus",2);W([b()],H.prototype,"_flowStatus",2);W([b()],H.prototype,"_downloadStatus",2);W([b()],H.prototype,"_mediaUploading",2);W([b()],H.prototype,"_flowUploading",2);W([b()],H.prototype,"_downloading",2);W([b()],H.prototype,"_mediaProgress",2);W([b()],H.prototype,"_flowProgress",2);W([b()],H.prototype,"_columnCount",2);H=W([w("flow-settings-panel")],H);var aw=Object.defineProperty,lw=Object.getOwnPropertyDescriptor,Ui=(e,t,i,r)=>{for(var o=r>1?void 0:r?lw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&aw(t,i,o),o};let Dt=class extends y{constructor(){super(...arguments),this.open=!1,this.title="",this.text="",this.confirmText="删除",this.cancelText="取消",this.destructive=!1,this._backdropDismiss=Mr(()=>this._cancel())}_cancel(){this.dispatchEvent(new CustomEvent("dialog-result",{bubbles:!0,composed:!0,detail:{confirmed:!1}}))}_confirm(){this.dispatchEvent(new CustomEvent("dialog-result",{bubbles:!0,composed:!0,detail:{confirmed:!0}}))}render(){return this.open?u`
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
          ${this.text?u`<p id="confirm-text" class="text">${this.text}</p>`:m}
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
    `:m}};Dt.styles=$`
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
  `;Ui([p({type:Boolean})],Dt.prototype,"open",2);Ui([p()],Dt.prototype,"title",2);Ui([p()],Dt.prototype,"text",2);Ui([p()],Dt.prototype,"confirmText",2);Ui([p()],Dt.prototype,"cancelText",2);Ui([p({type:Boolean})],Dt.prototype,"destructive",2);Dt=Ui([w("flow-confirm-dialog")],Dt);var cw=Object.defineProperty,dw=Object.getOwnPropertyDescriptor,kl=(e,t,i,r)=>{for(var o=r>1?void 0:r?dw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&cw(t,i,o),o};let Do=class extends y{constructor(){super(...arguments),this.open=!1,this._backdropDismiss=Mr(()=>this._cancel())}_cancel(){this.dispatchEvent(new CustomEvent("dialog-result",{bubbles:!0,composed:!0,detail:{confirmed:!1}}))}_confirm(){this.dispatchEvent(new CustomEvent("dialog-result",{bubbles:!0,composed:!0,detail:{confirmed:!0}}))}render(){return this.open?u`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog">
          ${this.cardConfig?u`
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
    `:m}};Do.styles=$`
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
  `;kl([p({type:Boolean})],Do.prototype,"open",2);kl([p({attribute:!1})],Do.prototype,"cardConfig",2);Do=kl([w("flow-delete-card-dialog")],Do);var hw=Object.defineProperty,uw=Object.getOwnPropertyDescriptor,Zo=(e,t,i,r)=>{for(var o=r>1?void 0:r?uw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&hw(t,i,o),o};const pw="Flow Dashboard",Qc="flow_sidebar_expanded";let Mi=class extends y{constructor(){super(...arguments),this.activePanel="home",this.connected=!1,this.narrow=!1,this._expanded=!0}connectedCallback(){super.connectedCallback();try{const e=localStorage.getItem(Qc);e!==null&&(this._expanded=e==="true")}catch{}}get _isExpanded(){return this.narrow||this._expanded}_toggleExpanded(){if(this.narrow){Z(this,"hass-toggle-menu",{open:!1});return}this._expanded=!this._expanded;try{localStorage.setItem(Qc,String(this._expanded))}catch{}Z(this,"hass-dock-sidebar",{dock:this._expanded?"docked":"auto"})}_select(e){this.dispatchEvent(new CustomEvent("panel-selected",{bubbles:!0,composed:!0,detail:{panel:e}})),this.narrow&&Z(this,"hass-toggle-menu",{open:!1})}_renderNavItem(e){const t=this.activePanel===e.panel;return u`
      <button
        type="button"
        class="nav-item ${t?"selected":""}"
        title=${e.label}
        aria-current=${t?"page":"false"}
        @click=${()=>this._select(e.panel)}
      >
        <ha-svg-icon class="nav-icon" .path=${e.iconPath}></ha-svg-icon>
        <span class="nav-label">${e.label}</span>
        ${e.badge?u`<span class="nav-badge">${e.badge}</span>`:m}
      </button>
    `}render(){const e=this._isExpanded,t=e?od:_a,i=this.connected?void 0:"!";return u`
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
          <span class="menu-title">${pw}</span>
        </div>

        <div class="sidebar-body">
          <div class="panel-list top-list">
            ${this._renderNavItem({panel:"home",iconPath:sd,label:"Home"})}
          </div>

          <div class="spacer"></div>

          <div class="panel-list bottom-list">
            ${this._renderNavItem({panel:"config",iconPath:nd,label:"设置",badge:i})}
          </div>
        </div>
      </aside>
    `}};Mi.styles=$`
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
  `;Zo([p({attribute:!1})],Mi.prototype,"activePanel",2);Zo([p({type:Boolean})],Mi.prototype,"connected",2);Zo([p({type:Boolean,reflect:!0})],Mi.prototype,"narrow",2);Zo([b()],Mi.prototype,"_expanded",2);Mi=Zo([w("ha-sidebar")],Mi);var fw=Object.defineProperty,gw=Object.getOwnPropertyDescriptor,Sl=(e,t,i,r)=>{for(var o=r>1?void 0:r?gw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&fw(t,i,o),o};let Io=class extends y{constructor(){super(...arguments),this.type="",this.open=!1,this._touchStartX=0,this._onTouchStart=e=>{var t;!this._modal||!this.open||(this._touchStartX=((t=e.touches[0])==null?void 0:t.clientX)??0)},this._onTouchEnd=e=>{var i;if(!this._modal||!this.open)return;const t=((i=e.changedTouches[0])==null?void 0:i.clientX)??this._touchStartX;this._touchStartX-t>72&&this._close()}}get _modal(){return this.type==="modal"}_close(){this.open&&(this.open=!1,Z(this,"hass-drawer-closed"))}updated(){document.body.style.overflow=this._modal&&this.open?"hidden":""}disconnectedCallback(){document.body.style.overflow="",super.disconnectedCallback()}render(){return this._modal?u`
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
      `:u`
      <div class="layout">
        <aside class="sidebar-shell">
          <slot name="sidebar"></slot>
        </aside>
        <div class="app-content">
          <slot></slot>
        </div>
      </div>
    `}};Io.styles=$`
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
  `;Sl([p({reflect:!0})],Io.prototype,"type",2);Sl([p({type:Boolean,reflect:!0})],Io.prototype,"open",2);Io=Sl([w("ha-drawer")],Io);var mw=Object.defineProperty,vw=Object.getOwnPropertyDescriptor,Cl=(e,t,i,r)=>{for(var o=r>1?void 0:r?vw(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&mw(t,i,o),o};let Mo=class extends y{constructor(){super(...arguments),this.narrow=!1,this.hasNotifications=!1}render(){return this.narrow?u`
      <ha-icon-button
        .path=${_a}
        label="菜单"
        @click=${this._toggleMenu}
      ></ha-icon-button>
      ${this.hasNotifications?u`<span class="dot"></span>`:m}
    `:m}_toggleMenu(){Z(this,"hass-toggle-menu")}};Mo.styles=$`
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
  `;Cl([p({type:Boolean})],Mo.prototype,"narrow",2);Cl([p({type:Boolean})],Mo.prototype,"hasNotifications",2);Mo=Cl([w("ha-menu-button")],Mo);var bw=Object.defineProperty,_w=Object.getOwnPropertyDescriptor,U=(e,t,i,r)=>{for(var o=r>1?void 0:r?_w(t,i):t,n=e.length-1,s;n>=0;n--)(s=e[n])&&(o=(r?s(t,i,o):s(o))||o);return r&&o&&bw(t,i,o),o};const Jc="flow_sidebar_expanded",yw="(max-width: 870px)";let F=class extends y{constructor(){super(...arguments),this._activeView=0,this._panel="home",this._editMode=!1,this._pickerOpen=!1,this._cardEditorOpen=!1,this._sectionEditorOpen=!1,this._viewEditorOpen=!1,this._viewEditorIsNew=!1,this._confirmOpen=!1,this._deleteCardOpen=!1,this._layoutEditEnabled=!1,this._layoutRevision=0,this._narrow=!1,this._drawerOpen=!1,this._sidebarExpanded=!0,this._connected=!1,this._applyingUndoRedo=!1,this._syncingHash=!1,this._undoRedo=new Gb({currentConfig:()=>({viewIndex:this._activeView,config:structuredClone(this.lovelace)}),apply:e=>this._applyUndoRedo(e),onStackChange:()=>this.requestUpdate()}),this._onKeyDown=e=>{!this._editMode||this._panel!=="home"||!(e.ctrlKey||e.metaKey)||(e.key==="z"&&!e.shiftKey?(e.preventDefault(),this._undo()):(e.key==="y"||e.key==="z"&&e.shiftKey)&&(e.preventDefault(),this._redo()))},this._onShowConfirm=e=>{this._confirmParams=e.detail,this._confirmOpen=!0},this._onShowDeleteCard=e=>{const t=e.detail;this._deleteCardConfig=t.cardConfig,this._deleteCardOpen=!0},this._onLayoutEditEnabled=e=>{var i;const t=!!((i=e.detail)!=null&&i.enabled);this._layoutEditEnabled=t,!t&&this._editMode&&this._exitEditMode()},this._onHashChange=()=>{this._syncingHash||this._syncViewFromHash()},this._onViewSelected=e=>{this._panel="home",this._selectView(e.detail.index)},this._onViewAdd=()=>{!this.lovelace||!this._editMode||(this._viewEditorIsNew=!0,this._viewEditorTarget=void 0,this._viewEditorOpen=!0)},this._onViewEdit=e=>{!this.lovelace||!this._editMode||(this._viewEditorIsNew=!1,this._viewEditorTarget=e.detail.index,this._viewEditorOpen=!0)},this._onViewDelete=e=>{this._deleteView(e.detail.index)},this._onLayoutWillChange=()=>{this._commitBeforeChange()},this._onLayoutChanged=()=>{this._notifyLayoutChanged()},this._onPanelSelected=e=>{this._editMode&&(this._editMode=!1,this._undoRedo.reset()),this._panel=e.detail.panel,this._narrow&&(this._drawerOpen=!1),this._panel==="home"&&this._writeViewHash(this._activeView,!0)},this._onToggleMenu=e=>{const t=e.detail;if(this._narrow){this._drawerOpen=(t==null?void 0:t.open)??!this._drawerOpen;return}this._sidebarExpanded=(t==null?void 0:t.open)??!this._sidebarExpanded;try{localStorage.setItem(Jc,String(this._sidebarExpanded))}catch{}},this._onDockSidebar=e=>{var i;const t=(i=e.detail)==null?void 0:i.dock;this._sidebarExpanded=t==="docked"},this._onAddSection=()=>{const e=this._view();e&&(this._commitBeforeChange(),yb(e,gd(!0)),this._notifyLayoutChanged())},this._onAddCard=e=>{this._pickerSection=e.detail.section,this._pickerOpen=!0},this._onEditCard=e=>{this._cardEditorTarget=e.detail,this._cardEditorOpen=!0},this._onDuplicateCard=e=>{const t=this._view();if(!t)return;const i=t.sections[e.detail.sectionIndex];i&&(this._commitBeforeChange(),Ut(t,e.detail.sectionIndex,kb(i,e.detail.cardIndex)),this._notifyLayoutChanged())},this._onCopyCard=e=>{const t=this._view();if(!t)return;const i=t.sections[e.detail.sectionIndex],r=i==null?void 0:i.cards[e.detail.cardIndex];r&&Hb(structuredClone(r))},this._onDeleteCard=e=>{this._deleteCard(e)},this._onEditSection=e=>{this._sectionEditorTarget=e.detail.sectionIndex,this._sectionEditorOpen=!0},this._onDuplicateSection=e=>{const t=this._view();t&&(this._commitBeforeChange(),Cb(t,e.detail.sectionIndex),this._notifyLayoutChanged())},this._onDeleteSection=e=>{this._deleteSection(e)}}connectedCallback(){super.connectedCallback();try{const e=localStorage.getItem(Jc);e!==null&&(this._sidebarExpanded=e==="true")}catch{}this._layoutEditEnabled=ch(),document.addEventListener(sa,this._onLayoutEditEnabled),this._unsubMediaQuery=Xb(yw,e=>{this._narrow=e,e||(this._drawerOpen=!1)}),document.addEventListener("flow-show-confirm",this._onShowConfirm),document.addEventListener("flow-show-delete-card",this._onShowDeleteCard),this.addEventListener("add-section",this._onAddSection),this.addEventListener("add-card",this._onAddCard),this.addEventListener("layout-will-change",this._onLayoutWillChange),this.addEventListener("layout-changed",this._onLayoutChanged),this.addEventListener("ll-edit-card",this._onEditCard),this.addEventListener("ll-duplicate-card",this._onDuplicateCard),this.addEventListener("ll-copy-card",this._onCopyCard),this.addEventListener("ll-delete-card",this._onDeleteCard),this.addEventListener("ll-edit-section",this._onEditSection),this.addEventListener("ll-duplicate-section",this._onDuplicateSection),this.addEventListener("ll-delete-section",this._onDeleteSection),this.addEventListener("hass-toggle-menu",this._onToggleMenu),this.addEventListener("hass-dock-sidebar",this._onDockSidebar),window.addEventListener("keydown",this._onKeyDown),window.addEventListener("hashchange",this._onHashChange),this._syncViewFromHash()}disconnectedCallback(){var e,t;(e=this._unsubMediaQuery)==null||e.call(this),(t=this._unsubConnection)==null||t.call(this),document.removeEventListener("flow-show-confirm",this._onShowConfirm),document.removeEventListener("flow-show-delete-card",this._onShowDeleteCard),document.removeEventListener(sa,this._onLayoutEditEnabled),this.removeEventListener("add-section",this._onAddSection),this.removeEventListener("add-card",this._onAddCard),this.removeEventListener("layout-will-change",this._onLayoutWillChange),this.removeEventListener("layout-changed",this._onLayoutChanged),this.removeEventListener("ll-edit-card",this._onEditCard),this.removeEventListener("ll-duplicate-card",this._onDuplicateCard),this.removeEventListener("ll-copy-card",this._onCopyCard),this.removeEventListener("ll-delete-card",this._onDeleteCard),this.removeEventListener("ll-edit-section",this._onEditSection),this.removeEventListener("ll-duplicate-section",this._onDuplicateSection),this.removeEventListener("ll-delete-section",this._onDeleteSection),this.removeEventListener("hass-toggle-menu",this._onToggleMenu),this.removeEventListener("hass-dock-sidebar",this._onDockSidebar),window.removeEventListener("keydown",this._onKeyDown),window.removeEventListener("hashchange",this._onHashChange),super.disconnectedCallback()}updated(e){this.toggleAttribute("expanded",this._sidebarExpanded&&!this._narrow),this.toggleAttribute("modal",this._narrow),e.has("flow")&&this._bindConnection(),e.has("lovelace")&&this.lovelace&&(this._clampActiveView(),this._syncViewFromHash())}_view(){if(this.lovelace)return _b(this.lovelace,this._activeView)}_clampActiveView(){var e;if(!((e=this.lovelace)!=null&&e.views.length)){this._activeView=0;return}this._activeView>=this.lovelace.views.length&&(this._activeView=this.lovelace.views.length-1),this._activeView<0&&(this._activeView=0)}_hashViewPath(){const e=location.hash.replace(/^#\/?/,"").trim();if(!e)return;const t=decodeURIComponent(e.split(/[/?#]/)[0]??"");if(!(!t||t==="config"||t==="settings"))return t}_syncViewFromHash(){if(!this.lovelace)return;const e=this._hashViewPath();if(e===void 0){this._writeViewHash(this._activeView,!0);return}const t=pc(this.lovelace,e);t!==this._activeView&&(this._activeView=t,this._closeEditors())}_writeViewHash(e,t=!1){if(!this.lovelace)return;const i=this.lovelace.views[e];if(!i)return;const r=`#/${na(i,e)}`;location.hash!==r&&(this._syncingHash=!0,t?history.replaceState(null,"",`${location.pathname}${location.search}${r}`):location.hash=r,queueMicrotask(()=>{this._syncingHash=!1}))}_selectView(e,t){if(!this.lovelace)return;const i=pc(this.lovelace,e);if(i===this._activeView){this._writeViewHash(i,(t==null?void 0:t.replaceHash)??!1);return}this._activeView=i,this._closeEditors(),this._writeViewHash(i,(t==null?void 0:t.replaceHash)??!1)}_closeEditors(){this._pickerOpen=!1,this._pickerSection=void 0,this._cardEditorOpen=!1,this._cardEditorTarget=void 0,this._sectionEditorOpen=!1,this._sectionEditorTarget=void 0,this._viewEditorOpen=!1,this._viewEditorTarget=void 0,this._viewEditorIsNew=!1}_reservedViewPaths(e){return this.lovelace?this.lovelace.views.map((t,i)=>i===e?void 0:t.path).filter(t=>!!t):[]}_saveViewEdit(e){if(!this.lovelace||!this._editMode)return;const t=e.detail.config;if(this._commitBeforeChange(),e.detail.isNew){const r=Ul({title:t.title,path:t.path,icon:t.icon});r.max_columns=t.max_columns??4,t.dense_section_placement&&(r.dense_section_placement=!0);const o=Ob(this.lovelace,r);this._viewEditorOpen=!1,this._viewEditorIsNew=!1,this._viewEditorTarget=void 0,this._notifyLayoutChanged(),this._selectView(o);return}const i=this._viewEditorTarget??this._activeView;if(Db(this.lovelace,i,{title:t.title,path:t.path,icon:t.icon,max_columns:t.max_columns,dense_section_placement:t.dense_section_placement}),!t.dense_section_placement){const r=this.lovelace.views[i];r&&delete r.dense_section_placement}this._viewEditorOpen=!1,this._viewEditorTarget=void 0,this._notifyLayoutChanged(),this._writeViewHash(i,!0)}async _deleteView(e){if(!this.lovelace||!this._editMode||this.lovelace.views.length<=1)return;const t=this.lovelace.views[e];if(!t)return;const i=t.title||t.path||`视图 ${e+1}`;if(!await hc({title:"删除视图",text:`确定删除「${i}」及其所有分区与卡片？`,confirmText:"删除",destructive:!0}))return;this._commitBeforeChange();const o=Ab(this.lovelace,e);this._notifyLayoutChanged(),this._selectView(o,{replaceHash:!0})}_commitBeforeChange(){!this._editMode||!this.lovelace||this._applyingUndoRedo||(this._undoRedo.commit({viewIndex:this._activeView,config:structuredClone(this.lovelace)}),this.requestUpdate())}_notifyLayoutChanged(){this._layoutRevision+=1,this._persist()}_headerTitle(e){return this._panel==="config"?"设置":e}_applyUndoRedo(e){this._applyingUndoRedo=!0,this.lovelace=structuredClone(e.config),this._activeView=e.viewIndex,this._clampActiveView(),this._layoutRevision+=1,this._applyingUndoRedo=!1,this._writeViewHash(this._activeView,!0),this._persist()}_undo(){this._undoRedo.undo()}_redo(){this._undoRedo.redo()}async _deleteCard(e){const t=this._view();if(!t)return;const i=t.sections[e.detail.sectionIndex];if(!i)return;const r=i.cards[e.detail.cardIndex];!e.detail.silent&&!await vb(r)||(this._commitBeforeChange(),Ut(t,e.detail.sectionIndex,Sb(i,e.detail.cardIndex)),this._notifyLayoutChanged())}_onConfirmResult(e){this._confirmOpen=!1,this._confirmParams=void 0,mb(e.detail.confirmed)}_onDeleteCardResult(e){this._deleteCardOpen=!1,this._deleteCardConfig=void 0,bb(e.detail.confirmed)}async _deleteSection(e){var o;const t=this._view();if(!t)return;const i=t.sections[e.detail.sectionIndex];!i||(((o=i.cards)==null?void 0:o.length)??0)>0&&!await hc({title:"删除部件",text:"此部件及其所有卡片都将被删除。",confirmText:"删除",destructive:!0})||(this._commitBeforeChange(),Eb(t,e.detail.sectionIndex),this._notifyLayoutChanged())}async _pickCard(e){var o;const t=this._view();if(!this._pickerSection||!t)return;const i=t.sections.indexOf(this._pickerSection);if(i<0)return;let r;if(e.config)r=structuredClone(e.config);else{const n=await ya(e.type);r=((o=n==null?void 0:n.getStubConfig)==null?void 0:o.call(n))??{type:e.type}}jb(r.type),this._commitBeforeChange(),Ut(t,i,xb(this._pickerSection,r)),this._pickerOpen=!1,this._pickerSection=void 0,this._notifyLayoutChanged()}async _saveCardEdit(e){const t=this._view(),i=this._cardEditorTarget;if(!t||!i)return;const r=t.sections[i.sectionIndex];r&&(this._commitBeforeChange(),Ut(t,i.sectionIndex,Pb(r,i.cardIndex,e.detail.config)),this._cardEditorOpen=!1,this._cardEditorTarget=void 0,this._notifyLayoutChanged())}_saveSectionEdit(e){const t=this._view(),i=this._sectionEditorTarget;!t||i===void 0||(this._commitBeforeChange(),Tb(t,i,e.detail.patch),this._sectionEditorOpen=!1,this._sectionEditorTarget=void 0,this._notifyLayoutChanged())}async _persist(){if(!this.lovelace)return;await Rb(this.lovelace);const e=ah(this.lovelace);this.flow.connection.trackAddresses(e),this.flow.connection.query(e)}_enterEditMode(){this._layoutEditEnabled&&(this._editMode=!0,this._undoRedo.reset())}_exitEditMode(){this._editMode=!1,this._undoRedo.reset()}_bindConnection(){var e,t;if((e=this._unsubConnection)==null||e.call(this),this._unsubConnection=void 0,!((t=this.flow)!=null&&t.connection)){this._connected=!1;return}this._connected=this.flow.connected,this._unsubConnection=this.flow.connection.subscribeConnection(i=>{this._connected=i})}render(){var h,f,v,g,_;if(!this.lovelace||!this.flow)return m;const e=this.lovelace.views,t=e[this._activeView],i=this._cardEditorTarget&&t?t.sections[this._cardEditorTarget.sectionIndex]:void 0,r=i&&this._cardEditorTarget?i.cards[this._cardEditorTarget.cardIndex]:void 0,o=this._sectionEditorTarget!==void 0&&t?t.sections[this._sectionEditorTarget]:void 0,n=(t==null?void 0:t.title)??(t==null?void 0:t.path)??this.lovelace.title??"Flow",s=this._headerTitle(n),a=this._panel==="home",l=a&&this._editMode,c=e.length>1,d=a&&(l||c);return u`
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
                ${d&&!l?u`
                      <flow-view-tabs
                        .views=${e}
                        .activeIndex=${this._activeView}
                        @view-selected=${this._onViewSelected}
                      ></flow-view-tabs>
                    `:u`
                      <span>${s}</span>
                      ${l?u`<ha-icon icon="mdi:pencil" class="edit-pencil"></ha-icon>`:m}
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
                    .path=${this._connected?cp:dp}
                  ></ha-svg-icon>
                </span>
                ${a&&this._layoutEditEnabled?l?u`
                        <ha-icon-button
                          .path=${rd}
                          label="撤销"
                          .disabled=${!this._undoRedo.canUndo}
                          @click=${this._undo}
                        ></ha-icon-button>
                        <ha-icon-button
                          .path=${id}
                          label="重做"
                          .disabled=${!this._undoRedo.canRedo}
                          @click=${this._redo}
                        ></ha-icon-button>
                        <ha-icon-button
                          .path=${hp}
                          label="完成"
                          @click=${this._exitEditMode}
                        ></ha-icon-button>
                      `:u`
                        <ha-icon-button
                          .path=${ba}
                          label="编辑"
                          @click=${this._enterEditMode}
                        ></ha-icon-button>
                      `:m}
              </div>
            </div>
            ${l?u`
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
            ${this._panel==="home"&&t?u`
                  <flow-view
                    .config=${t}
                    .viewIndex=${this._activeView}
                    .editMode=${this._editMode}
                    .layoutRevision=${this._layoutRevision}
                  ></flow-view>
                `:this._panel==="config"?u`<flow-settings-panel .flow=${this.flow}></flow-settings-panel>`:m}
          </div>
        </div>
      </ha-drawer>

      <flow-card-picker
          .open=${this._pickerOpen}
          .flow=${this.flow}
          @close=${()=>{this._pickerOpen=!1,this._pickerSection=void 0}}
          @card-picked=${k=>void this._pickCard(k.detail)}
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
          .config=${this._viewEditorIsNew?Ul({title:"新建视图",path:"",icon:"mdi:view-dashboard"}):this.lovelace.views[this._viewEditorTarget??this._activeView]}
          .reservedPaths=${this._reservedViewPaths(this._viewEditorIsNew?void 0:this._viewEditorTarget??this._activeView)}
          @close=${()=>{this._viewEditorOpen=!1,this._viewEditorTarget=void 0,this._viewEditorIsNew=!1}}
          @save=${this._saveViewEdit}
        ></flow-view-editor>

        <flow-confirm-dialog
          .open=${this._confirmOpen}
          .title=${((h=this._confirmParams)==null?void 0:h.title)??""}
          .text=${((f=this._confirmParams)==null?void 0:f.text)??""}
          .confirmText=${((v=this._confirmParams)==null?void 0:v.confirmText)??"删除"}
          .cancelText=${((g=this._confirmParams)==null?void 0:g.cancelText)??"取消"}
          .destructive=${((_=this._confirmParams)==null?void 0:_.destructive)??!1}
          @dialog-result=${this._onConfirmResult}
        ></flow-confirm-dialog>

        <flow-delete-card-dialog
          .open=${this._deleteCardOpen}
          .cardConfig=${this._deleteCardConfig}
          @dialog-result=${this._onDeleteCardResult}
        ></flow-delete-card-dialog>
    `}};F.styles=$`
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
  `;U([Kp({context:zi}),p({attribute:!1})],F.prototype,"flow",2);U([p({attribute:!1})],F.prototype,"lovelace",2);U([b()],F.prototype,"_activeView",2);U([b()],F.prototype,"_panel",2);U([b()],F.prototype,"_editMode",2);U([b()],F.prototype,"_pickerOpen",2);U([b()],F.prototype,"_pickerSection",2);U([b()],F.prototype,"_cardEditorOpen",2);U([b()],F.prototype,"_cardEditorTarget",2);U([b()],F.prototype,"_sectionEditorOpen",2);U([b()],F.prototype,"_sectionEditorTarget",2);U([b()],F.prototype,"_viewEditorOpen",2);U([b()],F.prototype,"_viewEditorIsNew",2);U([b()],F.prototype,"_viewEditorTarget",2);U([b()],F.prototype,"_confirmOpen",2);U([b()],F.prototype,"_confirmParams",2);U([b()],F.prototype,"_deleteCardOpen",2);U([b()],F.prototype,"_deleteCardConfig",2);U([b()],F.prototype,"_layoutEditEnabled",2);U([b()],F.prototype,"_layoutRevision",2);U([b()],F.prototype,"_narrow",2);U([b()],F.prototype,"_drawerOpen",2);U([b()],F.prototype,"_sidebarExpanded",2);U([b()],F.prototype,"_connected",2);F=U([w("flow-app")],F);class xw{constructor(){this.states=new Map,this.globalListeners=new Set,this.addressListeners=new Map}getStatesRecord(){const t={};return this.states.forEach((i,r)=>{t[r]=i}),t}getState(t){return this.states.get(t)}applyUpdate(t,i,r={}){var s;const o=this.states.get(t),n={address:t,state:i,attributes:{...o==null?void 0:o.attributes,...r},last_changed:Date.now()};this.states.set(t,n),(s=this.addressListeners.get(t))==null||s.forEach(a=>a(n)),this.notifyGlobal()}subscribe(t,i){this.addressListeners.has(t)||this.addressListeners.set(t,new Set),this.addressListeners.get(t).add(i);const r=this.states.get(t);return r&&i(r),()=>{var o;return(o=this.addressListeners.get(t))==null?void 0:o.delete(i)}}subscribeAll(t){return this.globalListeners.add(t),t(this.getStatesRecord()),()=>this.globalListeners.delete(t)}notifyGlobal(){const t=this.getStatesRecord();this.globalListeners.forEach(i=>i(t))}}class ww{constructor(t,i){this.url=i,this.connected=!1,this.ws=null,this.reconnectTimer=null,this.heartbeatTimer=null,this.connectionListeners=new Set,this.stateListeners=new Set,this.subscribedAddresses=new Set,this.store=t,this.store.subscribeAll(r=>{this.stateListeners.forEach(o=>o(r))})}connect(){this.ws||(this.ws=new WebSocket(this.url),this.ws.onopen=()=>{this.connected=!0,this.notifyConnection(),this.startHeartbeat(),this.subscribedAddresses.size>0&&this.query([...this.subscribedAddresses])},this.ws.onmessage=t=>{try{const i=JSON.parse(String(t.data));if(i.heartbeat)return;if(i.event==="actions_changed"){document.dispatchEvent(new CustomEvent("flow-actions-changed",{detail:i}));return}const r=i.address||i.addr;if(!r)return;this.store.applyUpdate(r,i.value),document.dispatchEvent(new CustomEvent("flow-ws-message",{detail:{address:r,value:i.value,raw:i}}))}catch{}},this.ws.onclose=()=>{this.connected=!1,this.ws=null,this.stopHeartbeat(),this.notifyConnection(),this.scheduleReconnect()},this.ws.onerror=()=>{this.connected=!1,this.notifyConnection()})}subscribeStates(t){return this.stateListeners.add(t),t(this.store.getStatesRecord()),()=>this.stateListeners.delete(t)}subscribeConnection(t){return this.connectionListeners.add(t),t(this.connected),()=>this.connectionListeners.delete(t)}trackAddresses(t){const i=t.filter(Boolean);i.forEach(r=>this.subscribedAddresses.add(r)),this.connected&&i.length>0&&this.query(i)}sendCommand(t,i){!this.ws||this.ws.readyState!==WebSocket.OPEN||this.ws.send(JSON.stringify({address:t,value:i}))}query(t){if(!this.ws||this.ws.readyState!==WebSocket.OPEN||t.length===0)return;const i=128;for(let r=0;r<t.length;r+=i)this.ws.send(JSON.stringify({query:t.slice(r,r+i)}))}notifyConnection(){this.connectionListeners.forEach(t=>t(this.connected))}scheduleReconnect(){this.reconnectTimer||(this.reconnectTimer=setTimeout(()=>{this.reconnectTimer=null,this.connect()},3e3))}startHeartbeat(){this.stopHeartbeat(),this.heartbeatTimer=setInterval(()=>{!this.ws||this.ws.readyState!==WebSocket.OPEN||this.ws.send(JSON.stringify({heartbeat:!0,t:Date.now()}))},25e3)}stopHeartbeat(){this.heartbeatTimer&&(clearInterval(this.heartbeatTimer),this.heartbeatTimer=null)}}function $w(e){const t={"/demo/floor_lamp":!0,"/demo/spotlights":70,"/demo/coffee":!1,"/demo/fridge":!0,"/demo/dishwasher":!1,"/demo/power":797.86,"/demo/voltage":235.61,"/demo/co2":458,"/demo/temperature":{value:10.2,attributes:{unit_of_measurement:"°C"}},"/demo/humidity":{value:56,attributes:{unit_of_measurement:"%"}},"/demo/string":{value:"Intro_Loop_v3.mp4",attributes:{friendly_name:"当前素材",icon:"mdi:filmstrip"}},"/demo/brightness":70,"/demo/trigger":!1,"/demo/switch":!1,"/demo/climate":{value:23,attributes:{friendly_name:"Upstairs",current_temperature:21.7}},"/demo/gain":{value:-18,attributes:{friendly_name:"输入增益",unit_of_measurement:"dB"}},"/demo/rgba":{value:[.12,.53,.9,1],attributes:{friendly_name:"RGBA",icon:"mdi:palette"}},"/demo/hsv":{value:[.58,.87,.9],attributes:{friendly_name:"HSV",icon:"mdi:palette-swatch"}},"/demo/faders":{value:[.25,.5,.75,.4],attributes:{friendly_name:"多维推杆",icon:"mdi:tune-vertical-variant"}},"/demo/xy":{value:[.5,.5],attributes:{friendly_name:"XY Pad",icon:"mdi:axis-arrow"}},"/demo/media":{value:!0,attributes:{friendly_name:"媒体播放器"}},"/demo/status_ws":{value:!0,attributes:{friendly_name:"connected",icon:"mdi:lan-connect"}},"/demo/status_tcp":{value:!0,attributes:{friendly_name:"tcp连接",icon:"mdi:lan-connect"}},"/demo/status_udp":{value:!0,attributes:{friendly_name:"connected",icon:"mdi:lan-connect"}},"/demo/status_osc":{value:!0,attributes:{friendly_name:"connected",icon:"mdi:lan-connect"}},"/demo/kitchen_shutter":{value:"open",attributes:{friendly_name:"Kitchen shutter",current_position:3}},"/demo/line3d":{value:"",attributes:{points:ay(120)}},"/demo/scatter3d":{value:"",attributes:{points:ly(80)}},"/demo/line2d":{value:"",attributes:{points:ny(80)}},"/demo/scatter2d":{value:"",attributes:{points:sy(60)}}};Object.entries(t).forEach(([i,r])=>{typeof r=="object"&&r!==null&&"value"in r?e.applyUpdate(i,r.value,r.attributes??{}):e.applyUpdate(i,r)})}async function kw(){go();const e=new xw,t=location.protocol==="https:"?"wss:":"ws:",i=new ww(e,`${t}//${location.host}/ws`),r=D_(e,i);$w(e);const o=await Nb(),n=ah(o),s=document.querySelector("flow-app");s instanceof HTMLElement&&(s.flow=r,s.lovelace=o,i.trackAddresses(n),i.connect(),i.subscribeStates(()=>{s.requestUpdate()}),i.subscribeConnection(()=>{s.requestUpdate()}),document.addEventListener("visibilitychange",()=>{document.visibilityState==="visible"&&i.query(n)}))}kw().catch(e=>{console.error("Flow bootstrap failed",e)});export{m as A,Vw as C,y as a,u as b,Xe as c,hn as d,zo as e,jw as f,Ie as g,$ as i,p as n,T as o,b as r,w as t};
