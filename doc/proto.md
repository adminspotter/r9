# r9 Network Protocol #

The r9 network protocol is intended to be:
* lightweight
* as stateless as possible

## Basic Packet ##

All packets are an extension of the basic packet.  It includes three
fields:

* type (unsigned byte)
* version (unsigned byte)
* sequence (unsigned 64-bit integer)

The `type` field holds the type of packet, a ping, an action request,
and so on.  Each type is defined in [proto.h](../proto/proto.h) as a
`TYPE_`*<type>*.

The `version` field notes the version of the protocol that the request
belongs to.  For this document, this field will always be `1`.

The `sequence` element can help with message ordering in an
out-of-order delivery condition, as is possible with UDP datagram
delivery.

The basic packet can function as a specific request, if no payload is
required.  The `PNGPKT` (ping) and `LGTREQ` (logout request) are such
types.

## Acknowledgement Packet ##

Any action may be acknowledged via an `ACKPKT`.

* type (unsigned byte)
* version (unsigned byte)
* sequence (unsigned 64-bit integer)
* request (unsigned byte)
* misc (4 x unsigned 64-bit integers)

## Client-initiated actions ##

These are actions which will only ever be initiated by a client.

### Login Request ###

<div style="float: right">
  <svg width="200" height="200" alt="Login transaction">
    <line x1="10" y1="0" x2="10" y2="200" style="stroke:black;stroke-width:2" />
    <line x1="190" y1="0" x2="190" y2="200" style="stroke:black;stroke-width:2" />
    <line x1="190" y1="30" x2="10" y2="60" style="stroke:black;stroke-width:2" />
    <polygon points="10,60 31,61 30,52" style="fill:black;stroke:black;stroke-width:1" />
    <line x1="190" y1="120" x2="10" y2="90" style="stroke:black;stroke-width:2" />
    <polygon points="190,120 169,121 170,112" style="fill:black;stroke:black;stroke-width:1" />
    <line x1="190" y1="160" x2="10" y2="130" style="stroke:black;stroke-width:2" />
    <polygon points="190,160 169,161 170,152" style="fill:black;stroke:black;stroke-width:1" />
    <text x="65" y="30" fill="black" transform="rotate(-10 140,30)">LOGREQ</text>
    <text x="65" y="90" fill="black" transform="rotate(10 60,100)">SRVKEY</text>
    <text x="65" y="130" fill="black" transform="rotate(10 60,160)">ACKPKT</text>
    Sorry, your browser does not support inline SVG.
  </svg>
</div>

* type (unsigned byte)
* version (unsigned byte)
* sequence (unsigned 64-bit integer)
* username (64-byte character string)
* charname (64-byte character string)
* pubkey (256-byte unsigned byte string)

### Logout Request ###

<div style="float: right">
  <svg width="200" height="150" alt="Logout transaction">
    <line x1="10" y1="0" x2="10" y2="150" style="stroke:black;stroke-width:2" />
    <line x1="190" y1="0" x2="190" y2="150" style="stroke:black;stroke-width:2" />
    <line x1="190" y1="30" x2="10" y2="60" style="stroke:black;stroke-width:2" />
    <polygon points="10,60 31,61 30,52" style="fill:black;stroke:black;stroke-width:1" />
    <line x1="190" y1="120" x2="10" y2="90" style="stroke:black;stroke-width:2" />
    <polygon points="190,120 169,121 170,112" style="fill:black;stroke:black;stroke-width:1" />
    <text x="65" y="30" fill="black" transform="rotate(-10 140,30)">LGTREQ</text>
    <text x="65" y="90" fill="black" transform="rotate(10 60,100)">ACKPKT</text>
    Sorry, your browser does not support inline SVG.
  </svg>
</div>

The logout request uses the basic packet, as it carries no payload.
The `type` field is set to `LGTREQ`.

### Action Request ###

<div style="float:right">
  <svg width="200" height="150" alt="Action request">
    <line x1="10" y1="0" x2="10" y2="150" style="stroke:black;stroke-width:2" />
    <line x1="190" y1="0" x2="190" y2="150" style="stroke:black;stroke-width:2" />
    <line x1="190" y1="30" x2="10" y2="60" style="stroke:black;stroke-width:2" />
    <polygon points="10,60 31,61 30,52" style="fill:black;stroke:black;stroke-width:1" />
    <line x1="190" y1="120" x2="10" y2="90" style="stroke:black;stroke-width:2" />
    <polygon points="190,120 169,121 170,112" style="fill:black;stroke:black;stroke-width:1" />
    <text x="65" y="30" fill="black" transform="rotate(-10 140,30)">ACTREQ</text>
    <text x="65" y="90" fill="black" transform="rotate(10 60,100)">ACKPKT</text>
    Sorry, your browser does not support inline SVG.
  </svg>
</div>

* type (unsigned byte)
* version (unsigned byte)
* sequence (64-bit unsigned integer)
* object_id (64-bit unsigned integer)
* action_id (16-bit unsigned integer)
* power_level (unsigned byte)
* x_pos_source (64-bit unsigned integer)
* y_pos_source (64-bit unsigned integer)
* z_pos_source (64-bit unsigned integer)
* dest_object_id (64-bit unsigned integer)
* x_pos_dest (64-bit signed integer)
* y_pos_dest (64-bit signed integer)
* z_pos_dest (64-bit signed integer)

## Server-initiated actions ##

### Server Key ###

This is the initial response to a [login request](#login%20request).

* type (unsigned byte)
* version (unsigned byte)
* sequence (64-bit unsigned integer)
* pubkey (256-byte unsigned byte string)
* iv (16-byte unsigned byte string)

### Position Update ###

<div style="float: right">
  <svg width="200" height="150" alt="Position update">
    <line x1="10" y1="0" x2="10" y2="150" style="stroke:black;stroke-width:2" />
    <line x1="190" y1="0" x2="190" y2="150" style="stroke:black;stroke-width:2" />
    <line x1="10" y1="30" x2="190" y2="60" style="stroke:black;stroke-width:2" />
    <polygon points="190,60 170,61 171,52" style="fill:black;stroke:black;stroke-width:1" />
    <text x="65" y="30" fill="black" transform="rotate(10 60,30)">POSUPD</text>
    Sorry, your browser does not support inline SVG.
  </svg>
</div>

* type (unsigned byte)
* version (unsigned byte)
* sequence (64-bit unsigned integer)
* object_id (64-bit unsigned integer)
* frame_number (16-bit unsigned integer)
* x_pos (64-bit unsigned integer)
* y_pos (64-bit unsigned integer)
* z_pos (64-bit unsigned integer)
* x_orient (32-bit unsigned integer)
* y_orient (32-bit unsigned integer)
* z_orient (32-bit unsigned integer)
* w_orient (32-bit unsigned integer)
* x_look (32-bit unsigned integer)
* y_look (32-bit unsigned integer)
* z_look (32-bit unsigned integer)

### Server Notification ###

This packet is still in the planning stages.  It currently has fields,
but they will almost certainly change once it's determined how these
packets should function.  Currently the server never sends it, and the
client mostly ignores any one it may happen to receive.

### Ping ###

<div style="float: right">
  <svg width="200" height="150" alt="Ping transaction">
    <line x1="10" y1="0" x2="10" y2="150" style="stroke:black;stroke-width:2" />
    <line x1="190" y1="0" x2="190" y2="150" style="stroke:black;stroke-width:2" />
    <line x1="10" y1="30" x2="190" y2="60" style="stroke:black;stroke-width:2" />
    <polygon points="190,60 170,61 171,52" style="fill:black;stroke:black;stroke-width:1" />
    <line x1="10" y1="120" x2="190" y2="90" style="stroke:black;stroke-width:2" />
    <polygon points="10,120 30,121 29,112" style="fill:black;stroke:black;stroke-width:1" />
    <text x="65" y="30" fill="black" transform="rotate(10 60,30)">PNGPKT</text>
    <text x="65" y="90" fill="black" transform="rotate(-10 140,100)">ACKPKT</text>
    Sorry, your browser does not support inline SVG.
  </svg>
</div>

If the client isn't doing much, they may not send any packets for
quite some time.  The server will automatically log the client out if
it determines the client is link-dead after a period.  After an
initial delay, the server will send a ping to make sure the client is
still there.
