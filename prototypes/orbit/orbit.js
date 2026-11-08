/* UI/audio sketch. No DAW sync or native DSP: the production plugin remains JUCE. */
'use strict';
const $=id=>document.getElementById(id);
const scales={minor:[57,59,60,62,64,65,67,69],major:[60,62,64,65,67,69,71,72],pentatonic:[57,60,62,64,67,69,72,74]};
const initial=[0,4,2,6,4,7,2,3];
let base=[...initial],notes=[...initial],locks=Array(8).fill(false),history=[],sound='glass',running=false,step=0,accentStep=0,context,master,timer,generation=0,selectedScale='minor';
const voices=new Set();
let effects,synthBus,recorderNode,recording=false,recordFinishing=false,recordChunks=[],recordSamples=0,wavUrl;
const controlValues=()=>Object.fromEntries(Object.keys(OrbitSession.ranges).map(id=>[id,Number($(id).value)]));
let audioClock,animationFrame,remainingBeats=0;
const visualQueue=[];
const straight=()=>Array.from({length:8},(_,index)=>({index,rest:false,beats:1}));
let flow=true,phrase=straight(),phraseNumber=1;
let kit,drumsEnabled=true,drumBeat=0,drumPattern=OrbitDrums.pattern(.45);
const drumEnabled={kick:true,clap:true,hat:true};
const noteName=m=>['Do','Do♯','Re','Re♯','Mi','Fa','Fa♯','Sol','Sol♯','La','La♯','Si'][m%12]+(Math.floor(m/12)-1);
const midi=d=>scales[$('scale').value][d];
const save=()=>{history.push({base:[...base],notes:[...notes],locks:[...locks],scale:selectedScale,flow,phrase:phrase.map(e=>({...e})),phraseNumber,drumPattern:drumPattern.map(h=>({...h}))});if(history.length>32)history.shift();};
function syncFlow(){
 $('flow-mode').setAttribute('aria-pressed',String(flow));$('loop-mode').setAttribute('aria-pressed',String(!flow));
 $('flow-description').textContent=flow?'La frase evolve da sola. Cattura la rende ripetibile.':'La frase corrente si ripete, comprese pause e durate.';
 $('phrase-count').textContent=`Frase ${phraseNumber} · ${phrase.length} passi`;
 $('cycle').textContent=flow?'Ritmo variabile':'Frase fissa';
}
function render(){
 const focused=document.activeElement;
 const focusCell=focused.closest?.('[data-step]');
 const focusIndex=focusCell?.dataset.step,focusTag=focused.tagName.toLowerCase();
 $('steps').replaceChildren();
 notes.forEach((n,i)=>{
  const cell=document.createElement('div');cell.className='step';cell.dataset.step=i;
  const number=document.createElement('span');number.className='step-number';number.textContent=String(i+1).padStart(2,'0');
  const select=document.createElement('select');select.setAttribute('aria-label',`Nota del passo ${i+1}`);
  scales[$('scale').value].forEach((m,j)=>select.add(new Option(noteName(m),j,false,j===n)));
  select.addEventListener('change',()=>{save();notes[i]=Number(select.value);base[i]=notes[i];render();$('steps').children[i].querySelector('select').focus();announce(`Passo ${i+1}: ${noteName(midi(notes[i]))}.`);});
  const lock=document.createElement('button');lock.className='lock';lock.textContent=locks[i]?'◆ Fisso':'◇ Libero';lock.setAttribute('aria-pressed',String(locks[i]));lock.setAttribute('aria-label',`${locks[i]?'Sblocca':'Blocca'} passo ${i+1}`);
  lock.onclick=()=>{save();locks[i]=!locks[i];render();$('steps').children[i].querySelector('button').focus();};
  cell.append(number,select,lock);$('steps').append(cell);
 });
 $('orbit-notes').innerHTML=notes.map((n,i)=>{const a=i*Math.PI/4-Math.PI/2;return `<g class="note-node ${locks[i]?'locked':''}" data-note="${i}"><circle cx="${300+193*Math.cos(a)}" cy="${250+193*Math.sin(a)}" r="26"/><text x="${300+193*Math.cos(a)}" y="${256+193*Math.sin(a)}">${noteName(midi(n))}</text></g>`;}).join('');
 const length=Number($('accent-length').value);
 $('orbit-accents').innerHTML=Array.from({length},(_,i)=>{const a=i*2*Math.PI/length-Math.PI/2;return `<circle class="accent-node ${i%2===0?'strong':''}" data-accent="${i}" cx="${300+135*Math.cos(a)}" cy="${250+135*Math.sin(a)}" r="${i%2===0?10:7}"/>`;}).join('');
 $('capture').disabled=!flow&&notes.every((n,i)=>n===base[i]);$('undo').disabled=!history.length;
 syncFlow();
 if(focusIndex!==undefined)$('steps').children[focusIndex]?.querySelector(focusTag)?.focus();
}
function announce(message){$('status').textContent=message;}
function stop(){finishRecording();running=false;generation++;clearInterval(timer);audioClock?.stop();cancelAnimationFrame(animationFrame);visualQueue.length=0;remainingBeats=0;kit?.stop();voices.forEach(v=>{try{v.stop();}catch{}});voices.clear();if(effects){effects.reset();effects.update(controlValues());}$('play').textContent='▶ Ascolta';$('playing-note').textContent='IN PAUSA';document.querySelectorAll('.current').forEach(e=>e.classList.remove('current'));}
function schedulePulse(time){
 const interval=30/Math.max(40,Math.min(200,Number($('bpm').value)||108));
 if(drumBeat%16===0&&flow&&Number($('amount').value)>0)drumPattern=OrbitDrums.pattern(Number($('density').value)/100);
 const hit=drumPattern[drumBeat%16];
 if(drumsEnabled)for(const kind of ['kick','clap','hat'])if(hit[kind]&&drumEnabled[kind])kit.trigger(kind,time,hit.velocity,Number($('depth').value)/100);
 drumBeat++;
 if(remainingBeats>0){remainingBeats--;return interval;}
 if(step>=phrase.length){
  step=0;
  if(flow){const next=OrbitFlow.createPhrase(notes,locks,Number($('amount').value)/100);notes=next.notes;phrase=next.events;phraseNumber++;render();}
 }
 const event=phrase[step],i=event.index,a=accentStep%Number($('accent-length').value);
 visualQueue.push({time,i,a,position:step+1,length:phrase.length,label:event.rest?'PAUSA':noteName(midi(notes[i]))});
 if(!event.rest){
 const now=time,osc=context.createOscillator(),gain=context.createGain(),filter=context.createBiquadFilter();
 const duration=sound==='mist'?.75:sound==='rubber'?.19:sound==='bell'?1.2:.5;
 osc.type=sound==='mist'?'sawtooth':sound==='rubber'?'triangle':'sine';
 if(sound==='keys')osc.setPeriodicWave(context.createPeriodicWave(new Float32Array([0,0,0]),new Float32Array([0,.75,.25])));osc.frequency.value=440*2**((midi(notes[i])-(sound==='rubber'?12:0)-69)/12);
 filter.type='lowpass';filter.frequency.value=300+Number($('tone').value)**2*1.3;filter.Q.value=sound==='rubber'?3:.5;
 gain.gain.setValueAtTime(0,now);gain.gain.linearRampToValueAtTime(a%2===0?.23:.12,now+(sound==='mist'?.08:.006));gain.gain.exponentialRampToValueAtTime(.0001,now+duration);
 if(sound==='bell'){
  const mod=context.createOscillator(),depth=context.createGain();mod.frequency.value=osc.frequency.value*2;
  depth.gain.setValueAtTime(osc.frequency.value*1.5,now);depth.gain.exponentialRampToValueAtTime(.01,now+duration);
  mod.connect(depth).connect(osc.frequency);mod.start(now);mod.stop(now+duration+.02);voices.add(mod);
  mod.onended=()=>{voices.delete(mod);mod.disconnect();depth.disconnect();};
 }
 const bassLevel=Number($('bass').value)/100;
 if(bassLevel>0){
  const sub=context.createOscillator(),subGain=context.createGain();sub.type='sine';sub.frequency.value=440*2**((midi(notes[i])-24-69)/12);
  const level=.22*bassLevel,duck=drumsEnabled&&drumEnabled.kick&&hit.kick?.35:1;
  subGain.gain.setValueAtTime(0,now);subGain.gain.linearRampToValueAtTime(level*duck,now+.008);subGain.gain.linearRampToValueAtTime(level*.7,now+.08);subGain.gain.exponentialRampToValueAtTime(.0001,now+.65);
  sub.connect(subGain).connect(effects.input);sub.start(now);sub.stop(now+.67);voices.add(sub);sub.onended=()=>{voices.delete(sub);sub.disconnect();subGain.disconnect();};
 }
 osc.connect(filter).connect(gain).connect(synthBus);osc.start(now);osc.stop(now+duration+.02);voices.add(osc);osc.onended=()=>{voices.delete(osc);osc.disconnect();filter.disconnect();gain.disconnect();};
 }
 step++;accentStep+=event.beats;remainingBeats=event.beats-1;return interval;
}
function drawPlayhead(){
 if(!running)return;
 const stamp=context.getOutputTimestamp?.();
 const audibleTime=stamp?.performanceTime>0?stamp.contextTime+(performance.now()-stamp.performanceTime)/1000:context.currentTime;
 let event;
 while(visualQueue.length&&visualQueue[0].time<=audibleTime)event=visualQueue.shift();
 if(event){
  document.querySelectorAll('.current').forEach(e=>e.classList.remove('current'));
  document.querySelector(`[data-note="${event.i}"]`)?.classList.add('current');
  document.querySelector(`[data-accent="${event.a}"]`)?.classList.add('current');
  $('steps').children[event.i]?.classList.add('current');
  $('step-display').textContent=`${String(event.position).padStart(2,'0')} / ${event.length}`;
  $('playing-note').textContent=event.label;
 }
 animationFrame=requestAnimationFrame(drawPlayhead);
}
async function play(){
 if(running){stop();return;}const token=++generation;
 try{context??=new (window.AudioContext||window.webkitAudioContext)();if(!master){master=context.createGain();master.connect(context.destination);effects=new OrbitEffects(context,master);synthBus=context.createGain();synthBus.connect(effects.input);}effects.update(controlValues());synthBus.gain.value=Number($('synth-volume').value)/100;kit??=new OrbitDrums.Kit(context,effects.input);kit.bus.gain.value=Number($('drum-volume').value)/100;master.gain.value=Number($('volume').value)/100;await context.resume();if(token!==generation)return;running=true;step=accentStep=drumBeat=0;remainingBeats=0;visualQueue.length=0;$('play').textContent='■ Ferma';audioClock=new OrbitAudioClock(context,schedulePulse);audioClock.start();timer=setInterval(()=>{if(context.state!=='running'){stop();return;}audioClock.pump();},25);drawPlayhead();}
 catch{stop();announce('Audio non disponibile. Apri il prototipo in Safari o Chrome e riprova.');}
}
$('play').onclick=play;
$('explore').onclick=()=>{const amount=Number($('amount').value)/100;if(!amount){announce('Esplora è a zero: la tua idea resta invariata.');return;}if(locks.every(Boolean)){announce('Tutti i passi sono fissi. Sblocca una nota per esplorare.');return;}save();notes=base.map((n,i)=>locks[i]?notes[i]:Math.random()<amount?Math.max(0,Math.min(7,n+(Math.random()<.5?-1:1)*Math.ceil(amount*4))):n);render();announce(notes.every((n,i)=>n===base[i])?'Questa variazione conserva l’idea. Puoi esplorare ancora.':'Nuova variazione pronta. Ascoltala, poi catturala.');};
$('capture').onclick=()=>{save();base=[...notes];flow=false;render();announce('Frase catturata, con pause e durate. Premi Flusso per ripartire.');};
$('undo').onclick=()=>{const previous=history.pop();if(!previous)return;({base,notes,locks,flow,phrase,phraseNumber,drumPattern}=previous);step=drumBeat=remainingBeats=0;selectedScale=previous.scale;$('scale').value=selectedScale;render();announce('Ultima modifica annullata.');};
$('reset').onclick=()=>{save();base=[...initial];notes=[...initial];locks.fill(false);phrase=straight();phraseNumber=1;step=remainingBeats=0;render();announce('Idea iniziale ripristinata. Puoi annullare.');};
$('flow-mode').onclick=()=>{if(flow)return;save();flow=true;render();announce('La frase evolverà al prossimo passaggio.');};
$('loop-mode').onclick=()=>{if(!flow)return;save();flow=false;render();announce('Evoluzione sospesa. La frase corrente si ripete.');};
$('scale').onchange=()=>{save();selectedScale=$('scale').value;render();announce('Scala aggiornata. I passi seguono la nuova scala.');};
$('accent-length').onchange=()=>{accentStep=0;render();};
for(const id of ['amount','tone','bass','volume'])$(id).oninput=()=>{$(`${id}-value`).textContent=`${$(id).value}%`;if(id==='volume'&&master)master.gain.setTargetAtTime(Number($(id).value)/100,context.currentTime,.02);};
$('bpm').onchange=()=>{$('bpm').value=Math.max(40,Math.min(200,Number($('bpm').value)||108));effects?.update(controlValues());};
document.querySelectorAll('[data-sound]').forEach(button=>button.onclick=()=>{sound=button.dataset.sound;document.querySelectorAll('[data-sound]').forEach(b=>b.setAttribute('aria-pressed',String(b===button)));$('sound-description').textContent={glass:'Attacchi nitidi, armoniche leggere.',rubber:'Bassi elastici, accenti percussivi.',mist:'Attacchi morbidi, trame sospese.',keys:'Tasti morbidi, armoniche rotonde.',bell:'Campane FM, code luminose.'}[sound];});
document.addEventListener('visibilitychange',()=>{if(document.hidden&&running)stop();});
window.addEventListener('pagehide',stop);
$('drums-toggle').onclick=()=>{drumsEnabled=!drumsEnabled;if(!drumsEnabled)kit?.stop();$('drums-toggle').setAttribute('aria-pressed',String(drumsEnabled));$('drums-toggle').textContent=drumsEnabled?'Ritmo attivo':'Ritmo spento';$('drum-status').textContent=drumsEnabled?'Il ritmo accompagna il synth.':'Solo synth. Riattiva il ritmo quando vuoi.';};
document.querySelectorAll('[data-drum]').forEach(button=>button.onclick=()=>{const kind=button.dataset.drum;drumEnabled[kind]=!drumEnabled[kind];button.setAttribute('aria-pressed',String(drumEnabled[kind]));if(!drumEnabled[kind])kit?.stop(kind);});
for(const id of ['density','depth','drum-volume'])$(id).oninput=()=>{$(`${id}-value`).textContent=`${$(id).value}%`;if(id==='density')drumPattern=OrbitDrums.pattern(Number($(id).value)/100);if(id==='drum-volume'&&kit)kit.bus.gain.setTargetAtTime(Number($(id).value)/100,context.currentTime,.02);};
render();

for(const id of ['synth-volume','delay-mix','delay-feedback','room-mix','room-size'])$(id).oninput=()=>{
 $(`${id}-value`).textContent=`${$(id).value}%`;
 if(id==='synth-volume')synthBus?.gain.setTargetAtTime(Number($(id).value)/100,context.currentTime,.02);else effects?.update(controlValues());
};
$('delay-division').onchange=()=>effects?.update(controlValues());
function download(blob,name){const url=URL.createObjectURL(blob),link=document.createElement('a');link.href=url;link.download=name;link.click();setTimeout(()=>URL.revokeObjectURL(url),60000);}
$('save-preset').onclick=()=>{
 const preset={format:'luma-orbit-web',version:1,base,notes,locks,sound,scale:selectedScale,flow,phrase,drumPattern,drumsEnabled,drumEnabled,controls:controlValues()};
 download(new Blob([JSON.stringify(OrbitSession.validate(preset),null,2)],{type:'application/json'}),'Orbit-preset.json');announce('Preset salvato nel file Orbit-preset.json.');
};
$('load-preset').onclick=()=>$('preset-file').click();
$('preset-file').onchange=async()=>{
 const file=$('preset-file').files[0];if(!file)return;
 try{
  if(file.size>65536)throw new Error('File troppo grande');const p=OrbitSession.validate(JSON.parse(await file.text()));
  stop();history=[];({base,notes,locks,sound,flow,phrase,drumPattern,drumsEnabled}=p);Object.assign(drumEnabled,p.drumEnabled);selectedScale=p.scale;$('scale').value=p.scale;
  for(const [id,value] of Object.entries(p.controls))if(Object.hasOwn(OrbitSession.ranges,id)){$(id).value=value;if($(`${id}-value`))$(`${id}-value`).textContent=`${value}%`;}
  document.querySelector(`[data-sound="${sound}"]`).click();
  $('drums-toggle').setAttribute('aria-pressed',String(drumsEnabled));$('drums-toggle').textContent=drumsEnabled?'Ritmo attivo':'Ritmo spento';
  document.querySelectorAll('[data-drum]').forEach(b=>b.setAttribute('aria-pressed',String(drumEnabled[b.dataset.drum])));
  $('drum-status').textContent=drumsEnabled?'Il ritmo accompagna il synth.':'Solo synth.';
  phraseNumber=1;step=accentStep=drumBeat=remainingBeats=0;render();announce('Preset caricato. Premi Ascolta.');
 }catch{announce('Preset non valido: scegli un file JSON salvato da Orbit web.');}finally{$('preset-file').value='';}
};
function finishRecording(){if(!recording)return;recording=false;recordFinishing=true;recorderNode.port.postMessage('stop');$('record').disabled=true;}
$('record').onclick=async()=>{
 if(recording){finishRecording();return;}if(recordFinishing)return;$('record').disabled=true;
 try{
  if(!running)await play();if(!running)throw new Error('Audio non disponibile');
  if(!recorderNode){
   await context.audioWorklet.addModule('recorder-worklet.js');recorderNode=new AudioWorkletNode(context,'orbit-recorder',{numberOfInputs:1,numberOfOutputs:1,outputChannelCount:[2]});master.connect(recorderNode);recorderNode.connect(context.destination);
   recorderNode.port.onmessage=({data})=>{
    if(data.chunk){recordChunks.push(data.chunk);recordSamples+=data.chunk.byteLength/2;$('record-status').textContent=`REC ${(recordSamples/2/context.sampleRate).toFixed(1)} s / 300 s`;}
    if(data.done){
     recording=false;recordFinishing=false;if(wavUrl)URL.revokeObjectURL(wavUrl);
     wavUrl=URL.createObjectURL(new Blob([OrbitSession.wavHeader(recordSamples,context.sampleRate),...recordChunks],{type:'audio/wav'}));recordChunks=[];
     const link=$('download-wav');link.href=wavUrl;link.download='Orbit.wav';link.hidden=false;
     $('record').disabled=false;$('record').textContent='● Registra WAV';$('record').setAttribute('aria-pressed','false');$('record-status').textContent=`WAV pronto: ${(data.frames/context.sampleRate).toFixed(1)} secondi. Premi Scarica WAV.`;
    }
   };
  }
  if(!running)throw new Error('Riproduzione interrotta');recordChunks=[];recordSamples=0;recording=true;recorderNode.port.postMessage('start');$('record').textContent='■ Termina registrazione';$('record').setAttribute('aria-pressed','true');$('record-status').textContent='REC · registrazione del mix in corso';
 }catch{$('record-status').textContent='Registrazione non disponibile: usa Orbit via HTTPS o localhost in un browser aggiornato.';}
 finally{if(!recordFinishing)$('record').disabled=false;}
};
