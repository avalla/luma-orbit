'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs');
const i18n=require('./i18n.js');
assert.deepEqual(Object.keys(i18n.catalog.languages),['it','en','fr','de','es']);
for(const [input,expected] of [['it-IT','it'],['fr_CA','fr'],['DE-de','de'],['es-MX','es'],['en-US','en'],['ja-JP','en'],[null,'en']])assert.equal(i18n.resolve(input),expected);
assert.equal(i18n.preferred({getItem:()=> 'de'},'it-IT'),'de');
assert.equal(i18n.preferred({getItem:()=> 'unsupported'},'fr-FR'),'fr');
assert.equal(i18n.preferred({getItem:()=>{throw Error('Storage blocked');}},'es-ES'),'es');
for(const [key,translations] of Object.entries(i18n.catalog.messages)){
 const placeholders=s=>(s.match(/\{\w+\}/g)||[]).sort();
 for(const lang of ['en','fr','de','es']){assert(translations[lang],`${lang}: ${key}`);assert.deepEqual(placeholders(translations[lang]),placeholders(key));}
}
for(const lang of Object.keys(i18n.catalog.languages)){
 const value=i18n.translate('Frase {number} · {length} passi',lang,{number:2,length:12});assert(value.includes('2')&&value.includes('12')&&!value.includes('{'));
 assert.equal(i18n.translate('Orbit',lang),'Orbit');
}
assert.equal(i18n.translate('Ascolta','xx'),'Play');
// Static copy cannot silently fall back to Italian when new UI text is added.
const html=fs.readFileSync(__dirname+'/index.html','utf8');
const unchanged=new Set(['luma','/ orbit','ORBIT 0.5','ORBIT','BPM','Glass','Rubber','Mist','Soft Keys','Bell','Clap','Hi-hat','Intro','Groove','Swing',...Object.values(i18n.catalog.languages)]);
const staticText=[...html.matchAll(/>([^<>]+)</g)].map(m=>m[1].trim()).filter(s=>/[A-Za-zÀ-ÿ]/.test(s));
const labels=[...html.matchAll(/aria-label="([^"]+)"/g)].map(m=>m[1]);
for(const key of [...staticText,...labels])assert(unchanged.has(key)||Object.hasOwn(i18n.catalog.messages,key),`Missing catalog message: ${key}`);
console.log('PASS: five complete catalogs, placeholders, locale fallback, blocked storage and static/accessibility copy');
