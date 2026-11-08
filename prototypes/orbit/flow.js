/* Pure phrase generation with injectable randomness for invariant checks. */
(function(root){
 function createPhrase(notes,locks,amount,random=Math.random){
  const strength=Math.max(0,Math.min(1,amount));
  const evolving=strength>0&&!locks.every(Boolean);
  const length=evolving?[8,12,16][Math.floor(random()*3)]:8;
  const next=[...notes];
  if(evolving){
   const candidates=notes.map((_,i)=>i).filter(i=>!locks[i]);
   const count=Math.max(1,Math.ceil(strength*3));
   for(let j=0;j<count&&candidates.length;j++){
    const i=candidates.splice(Math.floor(random()*candidates.length),1)[0];
    const delta=(random()<.5?-1:1)*(1+Math.floor(random()*Math.ceil(strength*3)));
    next[i]=Math.max(0,Math.min(7,next[i]+delta));
    if(next[i]===notes[i])next[i]=notes[i]===7?6:notes[i]+1;
   }
  }
  const events=Array.from({length},(_,j)=>{
   const index=j<8||!evolving?j%8:Math.floor(random()*8);
   const movable=evolving&&!locks[index];
   return {index,rest:movable&&j>0&&random()<strength*.28,beats:movable&&random()<strength*.35?2:1};
  });
  return {notes:next,events};
 }
 const api={createPhrase};
 if(typeof module!=='undefined'&&module.exports)module.exports=api;
 else root.OrbitFlow=api;
})(globalThis);
