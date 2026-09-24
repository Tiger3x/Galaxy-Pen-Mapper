import fs from 'node:fs';
import path from 'node:path';
const folder=process.argv[2];
if(!folder) throw new Error('Specify capture folder. Read-only analysis.');
function read(name){
  const lines=fs.readFileSync(path.join(folder,name),'utf8').trimEnd().split(/\r?\n/);
  if(lines.shift()!=='session,sequence,request_token,begin_100ns,end_100ns,status,length,valid,raw_hex') throw new Error('Unexpected schema');
  return lines.map((line,i)=>{
    const c=line.split(','); if(c.length!==9) throw new Error('Unexpected columns');
    const r={line:i+2,session:c[0],seq:BigInt(c[1]),token:c[2],begin:BigInt(c[3]),end:BigInt(c[4]),status:Number(c[5]),length:Number(c[6]),valid:Number(c[7]),raw:c[8]};
    r.bytes=c[8] ? c[8].split(' ').map(x=>parseInt(x,16)) : [];
    r.pressure=r.bytes[6]|(r.bytes[7]<<8);
    r.ok=r.valid===1 && r.status<0x80000000 && r.length===15 && r.bytes.length===15 && r.bytes[0]===2 && r.pressure<=4095;
    return r;
  });
}
const a=read('after-samsung.csv'),b=read('before-samsung.csv');
const count=(map,key)=>{map[key]=(map[key]||0)+1;};
const hex=n=>n.toString(16).padStart(2,'0');
function summary(rows){
  const flags={},pressures={}; let invalid=0;
  for(const r of rows){if(!r.ok){invalid++;continue;}
    const flag=hex(r.bytes[1]); flags[flag]??={count:0,min:4095,max:0,saturated:0};
    const s=flags[flag];s.count++;s.min=Math.min(s.min,r.pressure);s.max=Math.max(s.max,r.pressure);if(r.pressure===4095)s.saturated++;
    count(pressures,r.pressure);
  }
  return {records:rows.length,invalid,flags,pressures};
}
const tokens=new Map();b.forEach((r,j)=>{if(!tokens.has(r.token))tokens.set(r.token,[]);tokens.get(r.token).push(j);});
for(const js of tokens.values())js.sort((x,y)=>b[x].begin<b[y].begin?-1:b[x].begin>b[y].begin?1:0);
const uses=Array(b.length).fill(0);
const candidates=a.map(r=>{
  const js=tokens.get(r.token)||[]; let lo=0,hi=js.length;
  while(lo<hi){const m=(lo+hi)>>1;if(b[js[m]].begin<r.begin)lo=m+1;else hi=m;}
  const found=[];
  for(let k=lo;k<js.length && b[js[k]].begin<=r.end;k++){
    const j=js[k],s=b[j];
    if(r.session!=='0' && r.session===s.session && s.begin<=s.end && s.end<=r.end){found.push(j);uses[j]++;}
  }
  return found;
});
const result={matched:0,equal:0,pressureChanged:0,otherChanged:0,invalid:0,ambiguous:0,unpairedUpper:0,unpairedLower:0,flagTransitions:{},changedByteCounts:Array(15).fill(0),matchedContacts:0,matchedContactSaturated:0,samples:[],ambiguityExamples:[]};
const used=new Set();
a.forEach((r,i)=>{
  const js=candidates[i];
  if(!js.length){result.unpairedUpper++;return;}
  if(js.length!==1 || uses[js[0]]!==1){result.ambiguous++;if(result.ambiguityExamples.length<3)result.ambiguityExamples.push({upperLine:r.line,lowerLines:js.map(j=>b[j].line),uses:js.map(j=>uses[j])});return;}
  const s=b[js[0]]; used.add(js[0]);result.matched++;
  if(!r.ok||!s.ok){result.invalid++;return;}
  count(result.flagTransitions,hex(s.bytes[1])+' -> '+hex(r.bytes[1]));
  const changed=r.bytes.map((v,k)=>v!==s.bytes[k]?k:-1).filter(k=>k>=0);
  for(const k of changed)result.changedByteCounts[k]++;
  if(!changed.length)result.equal++;
  if(r.pressure!==s.pressure)result.pressureChanged++;
  if(changed.some(k=>k!==6&&k!==7))result.otherChanged++;
  if(r.bytes[1]===0x2c){result.matchedContacts++;if(r.pressure===4095)result.matchedContactSaturated++;}
  if(result.samples.length<3 && changed.length)result.samples.push({upperLine:r.line,lowerLine:s.line,before:s.raw,after:r.raw});
});
result.unpairedLower=b.length-used.size;
console.log(JSON.stringify({before:summary(b),after:summary(a),comparison:result},null,2));
