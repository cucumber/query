interface WithId {
    id: string | number
}

export function comparatorById(a: WithId, b: WithId) {
    if ( a.id < b.id ){
        return -1;
    }
    if ( a.id > b.id ){
        return 1;
    }
    return 0;
}