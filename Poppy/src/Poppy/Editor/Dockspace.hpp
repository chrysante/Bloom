#pragma once



namespace poppy {

    class Dockspace {
    public:
		void display();
		
	private:
		void dockspace();
		void toolbar();
		
    private:
		float toolbarHeight = 50;
    };

}
