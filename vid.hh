#include <click/string.hh>
#include <click/glue.hh>
#include <click/straccum.hh>

#ifndef VEIL_VID_HH
#define VEIL_VID_HH

#define VID_LEN 6
 
CLICK_DECLS

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
			memcpy(_data, data, 6);
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
		
		inline int logical_distance(VID *v)
		{
			const uint8_t *vid = v->sdata();		 
			int dist = 0, count = 0;
			for(int i = 0; i < VID_LEN ; i++)
			{
				if(_data[i] == vid[i])
					dist += 8;
				else{
					uint8_t res = _data[i] ^ vid[i];
					//count leading 0's in xor result
					while(res)
					{
						count++;
						res = res >> 1;
					}
					dist += 8-count;
					break;
				}
			}
			return dist;
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

CLICK_ENDDECLS
#endif
