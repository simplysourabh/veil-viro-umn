#include <click/string.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <click/etheraddress.hh>

#ifndef VEIL_VID_HH
#define VEIL_VID_HH

#define VID_LEN 6
#define HOST_LEN 2
#define ACTIVE_VID_LEN 3

 
CLICK_DECLS

//TODO: move functions to cc

class VID {
	private:
		uint8_t _data[6];
		
		VID(uint8_t m)
                {
                	_data[0] = _data[1] = _data[2] = _data[3] = _data[4] = _data[5] = m;   
                }

	public:
		inline VID() 
		{
			_data[0] = _data[1] = _data[2] = _data[3] = _data[4] = _data[5] = 0;
		}

		explicit VID(const unsigned char *data) 
		{
			memcpy(_data, data, VID_LEN);
		}

		static VID make_broadcast() 
		{
			return VID(0xFF);
		}

		inline bool is_broadcast() const 
		{
			return _data[0] == 0xFF && _data[1] == 0xFF && _data[2] == 0xFF &&
				_data[3] == 0xFF && _data[4] == 0xFF && _data[5] == 0xFF;
		}

		inline unsigned char *data() 
		{
			return reinterpret_cast<unsigned char *>(_data);
		}

		inline const unsigned char *data() const 
		{
			return reinterpret_cast<const unsigned char *>(_data);
		}
	
		inline const uint8_t *sdata() const 
		{
			return _data;
		}

		inline String vid_string()
		{
			char vid[VID_LEN*2+1];
			snprintf(vid, (int)sizeof(vid), "%02x%02x%02x%02x%02x%02x", _data[0], _data[1], _data[2], _data[3], _data[4], _data[5]);
			vid[VID_LEN*2] = '\0';
			return String(vid);
		}

		//TODO: change.
		inline size_t hashcode() const 
		{ 
			return (_data[5] | ((size_t) _data[4] << 8) | ((size_t) _data[3] << 16) | ((size_t) _data[2] << 24) | ((size_t) _data[1] << 31))
				^ ((size_t) _data[0] << 9);
		}
		
		inline uint16_t logical_distance(VID *v)
		{
			const uint8_t *vid = v->sdata();		 
			uint16_t lprefix = 0, count = 0;
			for(uint16_t i = 0; i < VID_LEN ; i++)
			{
				if(_data[i] == vid[i])
					lprefix += 8;
				else{
					uint8_t res = _data[i] ^ vid[i];
					//count leading 0's in xor result
					while(res)
					{
						count++;
						res = res >> 1;
					}
					lprefix += 8-count;
					break;
				}
			}
			return VID_LEN*8 - lprefix;
		}

		static void generate_host_vid(VID* svid, EtherAddress* host, VID* hostvid)
		{
			uint16_t len = VID_LEN - HOST_LEN;			
			unsigned char hvid[VID_LEN];
			//first 4B of host VID = first 4B of switch VID
			memcpy(hvid, svid, len);
			//TODO:find a cleaner way to do this
			//generate last 2B of host VID based on host MAC
			unsigned char* ethaddr = host->data();
			hvid[4] = (ethaddr[0] << 2) | (ethaddr[1] >> 3) | ethaddr[2];
			hvid[5] = (ethaddr[3] >> 1) | (ethaddr[4] << 2) | (ethaddr[5] << 4);	
			memcpy(hostvid, hvid, 6);		
		} 

		void extract_switch_vid(VID* svid){
			memset(svid, 0, VID_LEN);
			memcpy(svid, _data, VID_LEN - HOST_LEN);
		} 

		void calculate_rdv_point(uint16_t k, VID *rvid){
			uint16_t copy_bits = VID_LEN*8 - k + 1;
			uint16_t count = 0;
			unsigned char vid[VID_LEN];

			memset(vid, 0, sizeof(vid));			
			
			while(copy_bits > 8){
				vid[count] = _data[count];
				count++;
				copy_bits -= 8;	
			}
			if(copy_bits > 0){
				uint8_t t = _data[count] >> (8 - copy_bits);
				t = t << (8 - copy_bits);
				vid[count] = t;
			}
			
			memcpy(rvid, vid, 6);
		}	

		//flips bit in place
		void flip_bit(uint16_t k){
			unsigned char tvid[VID_LEN];
			memcpy(tvid, _data, VID_LEN);
			
			for(uint16_t i = VID_LEN - 1; i >= 0; i--){
			        if(k > 8)
                			k -= 8;
        		        else {
        			        tvid[i] = tvid[i] ^ (1<< (k-1));
			                break;
			        }
			}
			memcpy(_data, tvid, VID_LEN);
		}
};

inline bool
operator==(const VID &a, const VID &b)
{
	return (a.sdata()[0] == b.sdata()[0]
		&& a.sdata()[1] == b.sdata()[1]
		&& a.sdata()[2] == b.sdata()[2]
		&& a.sdata()[3] == b.sdata()[3]
		&& a.sdata()[4] == b.sdata()[4]
		&& a.sdata()[5] == b.sdata()[5]);
}

inline bool 
operator!=(const VID &a, const VID &b)
{
	return !(a == b);
}


class RendezvousEdge{
	public:
		VID src;
		VID dest;
		inline RendezvousEdge(){}
		inline RendezvousEdge(RendezvousEdge *edge){ memcpy(&src, &(edge->src), 6); memcpy(&dest, &(edge->dest), 6);}
		inline RendezvousEdge(VID *vid1, VID *vid2){ memcpy(&src, vid1, 6); memcpy(&dest, vid2, 6);}
		~RendezvousEdge(){}
		
		//TODO: change.
		inline size_t hashcode() const 
		{ 
			return (src.hashcode() ^ dest.hashcode());
		}
		
};

inline bool
operator==(const RendezvousEdge &a, const RendezvousEdge &b)
{
	return (a.src == b.src && a.dest == b.dest);
}

inline bool 
operator!=(const RendezvousEdge &a, const RendezvousEdge &b)
{
	return !(a == b);
}

CLICK_ENDDECLS
#endif
