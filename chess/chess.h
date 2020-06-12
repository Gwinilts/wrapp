//
// Created by gwinilts on 02/06/20.
//

#ifndef CPPTESTAGAIN_CHESS_H
#define CPPTESTAGAIN_CHESS_H

#include "../gxplatform.h"
#include "../network/networklayer.h"
#include "../ws/websocket.h"


namespace chs {
    enum class msg_type: unsigned char {
    	challange = 0xA1,
    	occupied = 0xA2,
		accept = 0xA3,
		deny = 0xA4,
		init = 0xA5,
		draw = 0xA6,
		select = 0xA7,
		suggest = 0xA8,
    	MIN = 0xA1, MAX = 0xA7
    };

    inline bool isMsgType(unsigned char v) {
    	if (v < static_cast<unsigned char>(msg_type::MIN)) return false;
    	if (v > static_cast<unsigned char>(msg_type::MAX)) return false;
    	return true;
    }

    enum class piece: unsigned char {
    	pawn = 0x01,
    	bishop = 0x02,
		knight = 0x03,
		rook = 0x04,
		king = 0x05,
		queen = 0x06
    };

    enum class color: unsigned char {
    	black = 1,
    	white = 0
    };

    struct piece_t {
    	color side;
    	piece type;
    };

    inline bool set(char t, char pos) {
    	return t & (1 << (pos - 1));
    }

    inline unsigned char ptoc(piece_t p) {
    	if (p.side == color::white) {
    		return 0b01000000 | (static_cast<unsigned char>(p.type) & 0b00001111);
    	} else {
    		return 0b00001111 & static_cast<unsigned char>(p.type);
    	}
    }

    inline unsigned char ptoc(piece p, color side) {
    	if (side == color::white) {
    		return 0b01000000 | (static_cast<unsigned char>(p) & 0b00001111);
    	} else {
    		return 0b00001111 & static_cast<unsigned char>(p);
    	}
    }

    inline unsigned char shrink(unsigned char x, unsigned char y) {
    	return (x << 4) | y;
    }

    inline void unshrink(unsigned char t, unsigned char &x, unsigned char &y) {
    	y = 0b00001111 & t;
    	x = 0b00001111 & (t >> 4);
    }

    inline piece_t ctop(unsigned char p) {
    	piece_t _p;

    	if ((p & 0b01000000) > 0) {
    		_p.side = color::white;
    	} else {
    		_p.side = color::black;
    	}

    	_p.type = static_cast<piece>(0b00001111 & p);

    	return _p;
    }

    class board {
    	char tile[8][8];
    	char king;

    	unsigned char *lastSelect;
    	char sel_size;
    	unsigned char origin;

    	color side;
    public:
    	board();
    	~board();

    	void reset(color side);

    	char getPossibleMoves(unsigned char pos, unsigned char* &moves);

    	bool isFriendlyOccupied(unsigned char x, unsigned char y);
    	bool isEnemyOccupied(unsigned char x, unsigned char y);
    	bool isOccupied(unsigned char x, unsigned char y);

    	bool isPossible(char x, char y);

    	bool isFriendlyOccupied(unsigned char x, unsigned char y, char _tile[8][8]);
    	bool isOccupied(unsigned char x, unsigned char y, char _tile[8][8]);
    	bool isEnemyOccupied(unsigned char x, unsigned char y, char _tile[8][8]);

    	bool willCauseCheck(unsigned char pos, unsigned char x, unsigned char y); // checks if moving this peice will cause check
    	bool willEndCheck(unsigned char pos, unsigned char x, unsigned char y);
    	char getPieces(unsigned char* &pieces);

    	unsigned char getOrigin();

    	char select(unsigned char pos, unsigned char* &moves);
    	bool move(unsigned char pos);
    	void move(unsigned char pos, unsigned char dst);
    };


}

class networklayer;

class chessgame {
private:
	websocket * ws;
	networklayer * layer;

	bool hasGame;
	bool hasMove;
	char _o;
	char _m;
	short _turn;
	string challangee;
	string challanger;
	string opponent;
	chs::board b;
	short turn;


	long *_since;
	bool white;

	bool since(chs::msg_type, long time);

public:
	chessgame(websocket *ws, networklayer *nl);
	virtual ~chessgame();

	void drawBoard();

	void handleSelect(unsigned char x, unsigned char y);
	void handleClientMsg(chs::msg_type, size_t, char* data);
	void handleNetworkMsg(chs::msg_type, string sender, unsigned short length, char* data);

	void work();

	void stop();
};


#endif //CPPTESTAGAIN_CHESS_H
