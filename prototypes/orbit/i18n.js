/* Local interface preference only: never part of the audio graph or preset. */
(function(root){
'use strict';
const catalog=typeof module!=='undefined'?require('./translations.js'):root.OrbitTranslations;
function resolve(value){const code=String(value||'').toLowerCase().split(/[-_]/)[0];return Object.hasOwn(catalog.languages,code)?code:'en';}
function translate(key,language,params={}){
 const message=language==='it'?key:catalog.messages[key]?.[resolve(language)]??catalog.messages[key]?.en??key;
 return message.replace(/\{(\w+)\}/g,(match,name)=>Object.hasOwn(params,name)?String(params[name]):match);
}
function preferred(storage,browserLanguage){try{const saved=storage?.getItem('orbit.language');if(saved&&Object.hasOwn(catalog.languages,saved))return saved;}catch{}return resolve(browserLanguage);}
let language='en';const textNodes=[],attributes=[],bindings=new Map(),listeners=new Set();
const t=(key,params)=>translate(key,language,params);
function text(element,key,params={}){if(typeof element==='string')element=document.getElementById(element);if(!element)return;bindings.set(element,{key,params});element.textContent=t(key,params);}
function apply(){
 document.documentElement.lang=language;
 for(const [node,key,prefix,suffix] of textNodes)if(node.isConnected)node.nodeValue=prefix+t(key)+suffix;
 for(const [element,attribute,key] of attributes)if(element.isConnected)element.setAttribute(attribute,t(key));
 for(const [element,value] of bindings){if(element.isConnected)element.textContent=t(value.key,value.params);else bindings.delete(element);}
 const selector=document.getElementById('language');if(selector)selector.value=language;
}
function mount(){
 let storage;try{storage=root.localStorage;}catch{}
 language=preferred(storage,root.navigator?.language);
 const walk=document.createTreeWalker(document.documentElement,NodeFilter.SHOW_TEXT);
 let node;while((node=walk.nextNode())){if(['SCRIPT','STYLE'].includes(node.parentElement?.tagName))continue;const key=node.nodeValue.trim();if(Object.hasOwn(catalog.messages,key)){const prefix=node.nodeValue.match(/^\s*/)[0],suffix=node.nodeValue.match(/\s*$/)[0];textNodes.push([node,key,prefix,suffix]);}}
 for(const element of document.querySelectorAll('[aria-label],[title]'))for(const attribute of ['aria-label','title']){const key=element.getAttribute(attribute);if(Object.hasOwn(catalog.messages,key))attributes.push([element,attribute,key]);}
 apply();
}
function setLanguage(value){language=resolve(value);try{root.localStorage?.setItem('orbit.language',language);}catch{}apply();for(const listener of listeners)listener(language);}
const api={catalog,resolve,translate,preferred,t,text,mount,setLanguage,get language(){return language;},onChange:fn=>listeners.add(fn)};
if(typeof module!=='undefined')module.exports=api;else root.OrbitI18n=api;
})(globalThis);
