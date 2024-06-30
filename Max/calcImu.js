autowatch = 1;
inlets = 1;
outlets = 3;

var nq = undefined;
var lastTime = undefined;
var originInv = {x: 0, y: 0, z: 0, w: 1};
function quaternion_mult(q1,q2){
    return {
        x: q2.w*q1.x+q2.x*q1.w-q2.y*q1.z+q2.z*q1.y,
        y: q2.w*q1.y+q2.x*q1.z+q2.y*q1.w-q2.z*q1.x,
        z: q2.w*q1.z-q2.x*q1.y+q2.y*q1.x+q2.z*q1.w,
        w: q2.w*q1.w-q2.x*q1.x-q2.y*q1.y-q2.z*q1.z
    };
}

function quaternion_inverse(q1){
    return {x: -q1.x, y: -q1.y, z: -q1.z, w: q1.w};
}
function setOrigin(x, y, z, w) {
    turnNum = 0;
    originInv = quaternion_inverse({x: x, y: y, z: z, w: w});
}
function quat(x, y, z, w, currentTime) {
    var q = {
        x: x, y: y, z: z, w: w
    };

    q = quaternion_mult(originInv, q);

    var deltaTime;
    var deltaTimeValue;

    if (lastTime === undefined) {
        lastTime = currentTime;
        deltaTime = 0;
        deltaTimeValue = 0;
    } else {
        deltaTime = currentTime - lastTime;
        deltaTimeValue = deltaTime/1000; 
    }
    if (nq == undefined) {
        nq = q;
    }
    var angleAxis = {
        angle: 0,
        axis: {x: 0, y: 0, z: 0}
    };

    if (nq.x == q.x && nq.y == q.y && nq.z == q.z && nq.w == q.w) {

    } else {
        var qd = quaternion_mult(q, quaternion_inverse(nq));
        //if (1 < qd.w) {
            var d = Math.sqrt(qd.x*qd.x+qd.y*qd.y+qd.z*qd.z+qd.w*qd.w);
            qd.x = qd.x / d;
            qd.y = qd.y / d;
            qd.z = qd.z / d;
            qd.w = qd.w / d;
        //}
        angleAxis = quaternionToAngleAxis(qd);
    }

    var angles = quaternionToEuler(q, 'zxy');

    nq = q;
    outlet(2, angleAxis.axis.x, angleAxis.axis.y, angleAxis.axis.z, angleAxis.angle * deltaTimeValue);
    outlet(1, angles.x, angles.y, angles.z);
    outlet(0, q.x, q.y, q.z, q.w);
}


function twoaxisrot(r11, r12, r21, r31, r32) {
    var res = [];
    if (1 < r21) {
        r21 = 1;
    } else if (r21 < -1) {
        r21 = -1;
    }
    res[0] = Math.atan2( r11, r12 );
    res[1] = Math.acos( r21 );
    res[2] = Math.atan2( r31, r32 );
    return res;
}

function threeaxisrot(r11, r12, r21, r31, r32){
    var res = [];
    res[0] = Math.atan2( r31, r32 );
    res[1] = Math.asin( r21 );
    res[2] = Math.atan2( r11, r12 );
    return res;
}
function quaternionToEuler(q, rotSeq) {
    var res;
    switch(rotSeq){
    case 'zyx':
      res = threeaxisrot( 2*(q.x*q.y + q.w*q.z),
                     q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z,
                    -2*(q.x*q.z - q.w*q.y),
                     2*(q.y*q.z + q.w*q.x),
                     q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z);
      break;

    case 'zyz':
      res = twoaxisrot( 2*(q.y*q.z - q.w*q.x),
                   2*(q.x*q.z + q.w*q.y),
                   q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z,
                   2*(q.y*q.z + q.w*q.x),
                  -2*(q.x*q.z - q.w*q.y));
      break;

    case 'zxy':
      res = threeaxisrot( -2*(q.x*q.y - q.w*q.z),
                      q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z,
                      2*(q.y*q.z + q.w*q.x),
                     -2*(q.x*q.z - q.w*q.y),
                      q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z);
      break;

    case 'zxz':
      res = twoaxisrot( 2*(q.x*q.z + q.w*q.y),
                  -2*(q.y*q.z - q.w*q.x),
                   q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z,
                   2*(q.x*q.z - q.w*q.y),
                   2*(q.y*q.z + q.w*q.x));
      break;

    case 'yxz':
      res = threeaxisrot( 2*(q.x*q.z + q.w*q.y),
                     q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z,
                    -2*(q.y*q.z - q.w*q.x),
                     2*(q.x*q.y + q.w*q.z),
                     q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z);
      break;

    case 'yxy':
      res = twoaxisrot( 2*(q.x*q.y - q.w*q.z),
                   2*(q.y*q.z + q.w*q.x),
                   q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z,
                   2*(q.x*q.y + q.w*q.z),
                  -2*(q.y*q.z - q.w*q.x));
      break;

    case 'yzx':
      res = threeaxisrot( -2*(q.x*q.z - q.w*q.y),
                      q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z,
                      2*(q.x*q.y + q.w*q.z),
                     -2*(q.y*q.z - q.w*q.x),
                      q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z);
      break;

    case 'yzy':
      res = twoaxisrot( 2*(q.y*q.z + q.w*q.x),
                  -2*(q.x*q.y - q.w*q.z),
                   q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z,
                   2*(q.y*q.z - q.w*q.x),
                   2*(q.x*q.y + q.w*q.z));
      break;

    case 'xyz':
      res = threeaxisrot( -2*(q.y*q.z - q.w*q.x),
                    q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z,
                    2*(q.x*q.z + q.w*q.y),
                   -2*(q.x*q.y - q.w*q.z),
                    q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z);
      break;

    case 'xyx':
      res = twoaxisrot( 2*(q.x*q.y + q.w*q.z),
                  -2*(q.x*q.z - q.w*q.y),
                   q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z,
                   2*(q.x*q.y - q.w*q.z),
                   2*(q.x*q.z + q.w*q.y));
      break;

    case 'xzy':
      res = threeaxisrot( 2*(q.y*q.z + q.w*q.x),
                     q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z,
                    -2*(q.x*q.y - q.w*q.z),
                     2*(q.x*q.z + q.w*q.y),
                     q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z);
      break;

    case 'xzx':
      res = twoaxisrot( 2*(q.x*q.z - q.w*q.y),
                   2*(q.x*q.y + q.w*q.z),
                   q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z,
                   2*(q.x*q.z + q.w*q.y),
                  -2*(q.x*q.y - q.w*q.z));
      break;
   }
   return {
        x: res[0] / Math.PI,
        y: res[1] / Math.PI,
        z: res[2] / Math.PI
    };
}

function quaternionToAngleAxis(q) {
    var angle = 2 * Math.acos(q.w) / Math.PI;
    var axis = [];
    var s = Math.sqrt(1 - q.w * q.w);
    if (s === 0) {
        axis.x = q.x;
        axis.y = q.y;
        axis.z = q.z;
    } else {
        axis.x = q.x / s;
        axis.y = q.y / s;
        axis.z = q.z / s;
    }

    return {
        angle: angle,
        axis: axis
    };
}