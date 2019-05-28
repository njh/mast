/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  Copyright (C) 2003-2007 Nicholas J. Humfrey <njh@aelius.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef MAST_MIME_TYPE_H
#define MAST_MIME_TYPE_H



// Maximum number of mime type parameters
#define MAX_MIME_TYPE_PARAMS    (16)


class MastMimeTypeParam {

public:
    // Constructors
    MastMimeTypeParam( const char* name, const char* value );
    MastMimeTypeParam( const char* string );
    ~MastMimeTypeParam();

    const char* get_name() {
        return this->name;
    };
    const char* get_value() {
        return this->value;
    };

    void set_name( const char* name );
    void set_value( const char* value );

private:
    char* name;
    char* value;
};


class MastMimeType {

public:
    // Constructors
    MastMimeType();
    MastMimeType( const char* string );
    ~MastMimeType();

    // Methods
    MastMimeTypeParam* get_param( int n ) {
        return this->param[ n ];
    };
    int parse( const char* string );
    void set_param( const char* name, const char* value );
    void set_param( const char* pair );
    void print();

    const char* get_major() {
        return this->major;
    };
    const char* get_minor() {
        return this->minor;
    };


private:

    int istoken( const char c );

    // Format: major/minor;name1=value1;name2=value2
    char* major;
    char* minor;

    // Array of parameters
    MastMimeTypeParam* param[ MAX_MIME_TYPE_PARAMS ];

};



#endif //_MAST_MIME_TYPE_H_
