/* The timer only fills a queue; all sound starts use the audio clock. */
(function(root){
 class AudioClock{
  constructor(context,schedule){this.context=context;this.schedule=schedule;this.active=false;this.nextTime=0;this.lookahead=.1;this.lead=.02;}
  start(){this.nextTime=this.context.currentTime+this.lead;this.active=true;this.pump();}
  stop(){this.active=false;}
  pump(){
   if(!this.active)return;
   const now=this.context.currentTime;
   // A stall beyond the horizon cannot be repaired: resume without a burst.
   if(this.nextTime<now)this.nextTime=now+this.lead;
   while(this.active&&this.nextTime<now+this.lookahead){
    const duration=this.schedule(this.nextTime);
    if(!Number.isFinite(duration)||duration<=0){this.stop();throw new Error('Invalid audio clock interval');}
    this.nextTime+=duration;
   }
  }
 }
 if(typeof module!=='undefined'&&module.exports)module.exports=AudioClock;else root.OrbitAudioClock=AudioClock;
})(globalThis);
