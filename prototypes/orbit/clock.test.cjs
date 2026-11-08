const assert=require('node:assert/strict');
const AudioClock=require('./clock.js');
const context={currentTime:0},events=[];
let interval=.15;
const clock=new AudioClock(context,time=>{events.push(time);return interval;});
clock.start();
// Irregular polling must not move scheduled onsets or accumulate timer drift.
for(let i=0;i<3000;i++){context.currentTime+=[.017,.031,.024,.045][i%4];clock.pump();}
assert(events.length>500);
events.forEach((time,i)=>assert(Math.abs(time-(.02+i*.15))<1e-9));
const boundary=clock.nextTime;interval=.3;
while(events.at(-1)<boundary+.6){context.currentTime+=.025;clock.pump();}
const changed=events.filter(time=>time>=boundary-1e-9);
assert(Math.abs(changed[1]-changed[0]-.3)<1e-9);
const before=events.length;context.currentTime+=5;clock.pump();
assert.equal(events.length,before+1);assert(events.at(-1)>context.currentTime);
clock.stop();const stopped=events.length;context.currentTime+=1;clock.pump();assert.equal(events.length,stopped);
clock.start();assert.equal(events.length,stopped+1);assert(Math.abs(events.at(-1)-context.currentTime-.02)<1e-9);
console.log('PASS: 500+ drift-free onsets, tempo change, long-stall recovery without burst, stop and restart.');
