/*
*
*   Opened extension to Art-Net protocol designed in conjunction with MadMapper team and Advatek Lighting. Not officially supported.
*   Replace 'Art-Net' header with 'Art-Ext' header to show this is not the standard protocol.
*   Increment OpCode by 1 for each similar packet. This should be an extra gaurd against incompatible art-net devices that
*   mistakenly forget to drop the packet as soon as they see a different ID code.
*   Obtain all possible information from existing art-net packets, only use this extension where art-net fails.
*   I.E. Use in conjunction with a normal art-net poll reply for standard information already available.
*
*/

#ifdef _MSC_VER
        // ms VC .NET
        #pragma pack( push, before_definition )
        #pragma pack(1)
        #define ATTRIBUTE_PACKED ;
#else
        // gcc
        #define ATTRIBUTE_PACKED __attribute__( ( packed ) )
#endif

#define ARTNET_EXT_MIN_FRAME_SIZE 10
#define ARTNET_EXT_FRAME_ID         "Art-Ext"
#define ARTNET_EXT_FRAME_ID_LEN     8
#define ARTNET_EXT_PROT_VERS        1
#define ARTNET_EXT_OPCODE_POLL      (ARTNET_OPCODE_POLL+1)
#define ARTNET_EXT_OPCODE_POLL_REPLY (ARTNET_OPCODE_POLL_REPLY+1)

/*
*
*   Send this art-ext version of the poll to check if the devices can respond to the art-net extension protocol.
*   We don't need the usual TalkToMe or Priority fields as they will be obtained from the standard art-net poll.
*
*/

struct art_ext_frame_poll_t
{
        uint8_t                 id[ 8 ];                // 'A' 'r' 't' '-' 'E' 'x' 't' 0x00
        uint16_t                opcode;                 // 0x2001 (Art-Net OpPoll + 1)
        uint16_t				prot_ver;               // Protocol version (OF ART-EXT) = 1 currently
} ATTRIBUTE_PACKED;


/*
*
*   Extension to art-poll reply. Designed to allow unlimited ports to be reported for both input and output. The universe values
*   in these packets will over-ride the standard art-poll reply packets when available. Other information from standard art-poll-reply
*   packets will still be valid (such as node name, firmware version etc)
*
*   The 'response_num' and 'total_responses' are set to allow multiple art-ext poll reply packets to be sent. This allows unlimited
*   numbers of universes to be reported if this is required in the future.
*
*   E.G. If there are 600 output universes on a node, first packet will be 'response_num' = 1, 'total_responses' = 3 and contain the
*   first 255 universes. It will be followed by a packet with 'response_num' = 2, 'total_responses' = 3 with the next 255 universes.
*
*   The swin and swout fields are variable arrays that will be as long as the previous 'num_ports_in' and 'num_ports_out' have set.
*
*   There will not be a single 'High bits' field with an array of 'low bits' fields for the universe numbering. This methodology
*   in the normal art-net packets seriously limits the possible universe numbering in multiple ways. Instead, each swin or swout
*   value will be the full 15bit art-net universe number represented in a uint16_t field.
*
*/

struct art_ext_frame_poll_reply_t
{
        uint8_t			id[ 8 ];                // 'A' 'r' 't' '-' 'E' 'x' 't' 0x00
        uint16_t		opcode;                 // 0x2101 (Art-Net OpPollReply + 1)
        uint16_t		prot_ver;               // Protocol version (OF ART-EXT) = 1 currently
        uint32_t		ipv4_address;
        uint8_t			mac[ 6 ];

		uint8_t			response_num;           // This is packet number x of y
		uint8_t			total_resonses;         // There are y packets

        uint8_t			num_ports_in;	        // 0-255 input universes
        uint8_t			num_ports_out;	        // 0-255 output universes

        uint16_t		swinout;
} ATTRIBUTE_PACKED;

#if defined(_MSC_VER)
#pragma pack( pop, before_definition )
#endif