/* Synthesized percussion, no samples or network assets. */
(function(root){
 function pattern(density,random=Math.random){
  const d=Math.max(0,Math.min(1,density));
  return Array.from({length:16},(_,i)=>({
   kick:i===0||i===8||(i%2===0&&random()<d*.25),
   clap:i===4||i===12,
   hat:i%2===0||(random()<d),
   velocity:i%2===0?1:.6
  }));
 }
 class Kit{
  constructor(context,output){
   this.context=context;this.voices=new Set();this.bus=context.createGain();this.bus.gain.value=.55;this.bus.connect(output);
   this.noise=context.createBuffer(1,Math.ceil(context.sampleRate*.25),context.sampleRate);
   const samples=this.noise.getChannelData(0);for(let i=0;i<samples.length;i++)samples[i]=Math.random()*2-1;
  }
  trigger(kind,time,velocity,depth){
   const c=this.context,gain=c.createGain(),filter=c.createBiquadFilter();let source,duration;
   if(kind==='kick'){
    source=c.createOscillator();source.type='sine';duration=.18+depth*.32;
    source.frequency.setValueAtTime(135,time);source.frequency.exponentialRampToValueAtTime(42+(1-depth)*20,time+.065);
    filter.type='lowpass';filter.frequency.value=700;
    gain.gain.setValueAtTime(0,time);gain.gain.linearRampToValueAtTime(.75*velocity,time+.003);gain.gain.exponentialRampToValueAtTime(.0001,time+duration);
   }else{
    source=c.createBufferSource();source.buffer=this.noise;duration=kind==='hat'?.055:.18;
    filter.type=kind==='hat'?'highpass':'bandpass';filter.frequency.value=kind==='hat'?7500:1500;filter.Q.value=.7;
    const level=(kind==='hat'?.14:.5)*velocity;
    gain.gain.setValueAtTime(.0001,time);
    if(kind==='clap'){
     for(let n=0;n<3;n++){gain.gain.linearRampToValueAtTime(level,time+n*.014+.002);gain.gain.exponentialRampToValueAtTime(level*.15,time+n*.014+.012);}
    }else gain.gain.linearRampToValueAtTime(level,time+.002);
    gain.gain.exponentialRampToValueAtTime(.0001,time+duration);
   }
   source.connect(filter).connect(gain).connect(this.bus);source.start(time);source.stop(time+duration+.01);source.orbitKind=kind;this.voices.add(source);
   source.onended=()=>{this.voices.delete(source);source.disconnect();filter.disconnect();gain.disconnect();};
  }
  stop(kind){for(const voice of this.voices){if(!kind||voice.orbitKind===kind){try{voice.stop();}catch{}this.voices.delete(voice);}}}
 }
 const api={pattern,Kit};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.OrbitDrums=api;
})(globalThis);
